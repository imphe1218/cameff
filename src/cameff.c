#include "cameff.h"
#include "lbfgsb_standalone.h"

#include <float.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define CAMEFF_TRIGGER_MEMORY_DAYS 365
#define CAMEFF_MIN_EVENTS_FOR_MC 200
#define CAMEFF_MIN_EVENTS_ABOVE_MC 100

static const double CAMEFF_ALPHA_GRID[] = {0.5, 1.0, 1.5, 2.0};
static const double CAMEFF_C_GRID[] = {0.05, 0.10, 0.50};
static const double CAMEFF_P_GRID[] = {1.0, 1.2, 1.5};

double cameff_clip01(double x) {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}

double cameff_sigmoid(double x) {
    if (x >= 0.0) {
        double z = exp(-x);
        return 1.0 / (1.0 + z);
    }
    {
        double z = exp(x);
        return z / (1.0 + z);
    }
}

double cameff_probability_logit(double p) {
    if (p < CAMEFF_EPS) p = CAMEFF_EPS;
    if (p > 1.0 - CAMEFF_EPS) p = 1.0 - CAMEFF_EPS;
    return log(p / (1.0 - p));
}

double cameff_signal_effective(const cameff_signal_t *signal) {
    if (signal == NULL || !signal->available) return 0.0;
    return cameff_clip01(signal->value) * cameff_clip01(signal->reliability);
}

double cameff_weighted_quality(
    const cameff_signal_t *signals,
    const double *weights,
    size_t count,
    int *available
) {
    double numerator = 0.0;
    double denominator = 0.0;
    size_t i;

    if (available != NULL) *available = 0;
    if (signals == NULL || weights == NULL) return 0.0;

    for (i = 0; i < count; ++i) {
        if (weights[i] < 0.0) return 0.0;
        if (!signals[i].available) continue;
        numerator += weights[i] * cameff_clip01(signals[i].reliability);
        denominator += weights[i];
    }

    if (denominator <= 0.0) return 0.0;
    if (available != NULL) *available = 1;
    return cameff_clip01(numerator / denominator);
}

void cameff_normalize_supports(
    const double *raw,
    size_t count,
    double temperature,
    double *out
) {
    size_t i;
    double max_value = -DBL_MAX;
    double sum = 0.0;

    if (raw == NULL || out == NULL || count == 0 || temperature <= 0.0) return;

    for (i = 0; i < count; ++i) {
        double scaled = raw[i] / temperature;
        if (scaled > max_value) max_value = scaled;
    }
    for (i = 0; i < count; ++i) {
        out[i] = exp(raw[i] / temperature - max_value);
        sum += out[i];
    }
    if (sum <= 0.0) {
        for (i = 0; i < count; ++i) out[i] = 0.0;
        return;
    }
    for (i = 0; i < count; ++i) out[i] /= sum;
}

double cameff_unknown_support(
    double max_known,
    double quality,
    double nearest_domain_distance,
    int has_distance,
    double lambda_max,
    double lambda_quality,
    double lambda_distance,
    double distance_scale
) {
    double distance_term = 0.0;
    double value;

    if (has_distance && lambda_distance != 0.0) {
        double distance = nearest_domain_distance < 0.0 ? 0.0 : nearest_domain_distance;
        double scale = distance_scale > CAMEFF_EPS ? distance_scale : CAMEFF_EPS;
        distance_term = 1.0 - exp(-distance / scale);
    }

    value = lambda_max * (1.0 - cameff_clip01(max_known))
          + lambda_quality * (1.0 - cameff_clip01(quality))
          + lambda_distance * distance_term;
    return cameff_clip01(value);
}

cameff_availability_t cameff_fuse(
    cameff_expert_result_t *results,
    size_t count,
    double agreement_beta,
    double coherence_beta,
    double contradiction_beta,
    double quality_beta,
    double agreement,
    double coherence,
    double contradiction,
    double quality,
    double *score_out
) {
    double numerator = 0.0;
    double denominator = 0.0;
    double score;
    size_t i;

    if (score_out != NULL) *score_out = 0.0;
    if (results == NULL || score_out == NULL) return CAMEFF_INSUFFICIENT_EVIDENCE;

    for (i = 0; i < count; ++i) {
        cameff_expert_result_t *r = &results[i];
        if (r->status != CAMEFF_AVAILABLE) continue;
        if (r->effective_weight <= 0.0) continue;
        numerator += r->effective_weight * cameff_clip01(r->output);
        denominator += r->effective_weight;
    }

    if (denominator <= 0.0) return CAMEFF_INSUFFICIENT_EVIDENCE;

    score = numerator / denominator;
    score += agreement_beta * (agreement - 0.5);
    score += coherence_beta * (coherence - 0.5);
    score -= fabs(contradiction_beta) * contradiction;
    score *= fmax(0.0, 1.0 - quality_beta * (1.0 - quality));

    *score_out = cameff_clip01(score);
    return CAMEFF_AVAILABLE;
}

static int compare_double(const void *a, const void *b) {
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
}

cameff_availability_t cameff_estimate_mc(
    const cameff_event_t *events,
    size_t count,
    double decision_time_days,
    double *mc_out
) {
    double *magnitudes;
    size_t n = 0, i;
    double min_mag, max_mag, bin_start;
    size_t bins, *hist, max_index = 0;

    if (mc_out != NULL) *mc_out = 0.0;
    if (events == NULL || mc_out == NULL) return CAMEFF_INSUFFICIENT_EVIDENCE;

    magnitudes = (double *)malloc(count * sizeof(double));
    if (magnitudes == NULL) return CAMEFF_INSUFFICIENT_EVIDENCE;

    for (i = 0; i < count; ++i) {
        if (events[i].time_days >= decision_time_days) continue;
        if (!isfinite(events[i].magnitude)) continue;
        magnitudes[n++] = events[i].magnitude;
    }

    if (n < CAMEFF_MIN_EVENTS_FOR_MC) {
        free(magnitudes);
        return CAMEFF_INSUFFICIENT_EVIDENCE;
    }

    qsort(magnitudes, n, sizeof(double), compare_double);
    min_mag = floor(magnitudes[0] * 10.0) / 10.0;
    max_mag = ceil(magnitudes[n - 1] * 10.0) / 10.0;
    bin_start = min_mag;
    bins = (size_t)ceil((max_mag - min_mag) / 0.1) + 2;

    hist = (size_t *)calloc(bins, sizeof(size_t));
    if (hist == NULL) {
        free(magnitudes);
        return CAMEFF_INSUFFICIENT_EVIDENCE;
    }

    for (i = 0; i < n; ++i) {
        long index = (long)floor((magnitudes[i] - bin_start) / 0.1 + 1e-9);
        if (index < 0) index = 0;
        if ((size_t)index >= bins) index = (long)bins - 1;
        hist[index]++;
    }

    for (i = 1; i < bins; ++i) {
        if (hist[i] > hist[max_index]) max_index = i;
    }

    *mc_out = bin_start + (double)max_index * 0.1 + 0.2;
    *mc_out = round(*mc_out * 100.0) / 100.0;
    if (*mc_out < 1.0) *mc_out = 1.0;
    if (*mc_out > 5.0) *mc_out = 5.0;

    free(hist);
    free(magnitudes);
    return CAMEFF_AVAILABLE;
}

cameff_availability_t cameff_estimate_b_value(
    const cameff_event_t *events,
    size_t count,
    double decision_time_days,
    double mc,
    double *b_out,
    size_t *events_above_mc_out
) {
    size_t i, n = 0;
    double sum = 0.0;
    double mean, denominator;

    if (b_out != NULL) *b_out = 0.0;
    if (events_above_mc_out != NULL) *events_above_mc_out = 0;
    if (events == NULL || b_out == NULL) return CAMEFF_INSUFFICIENT_EVIDENCE;

    for (i = 0; i < count; ++i) {
        if (events[i].time_days >= decision_time_days) continue;
        if (!isfinite(events[i].magnitude)) continue;
        if (events[i].magnitude < mc) continue;
        sum += events[i].magnitude;
        n++;
    }

    if (events_above_mc_out != NULL) *events_above_mc_out = n;
    if (n < CAMEFF_MIN_EVENTS_ABOVE_MC) return CAMEFF_INSUFFICIENT_EVIDENCE;

    mean = sum / (double)n;
    denominator = mean - (mc - 0.05);
    if (denominator <= 0.0) return CAMEFF_INSUFFICIENT_EVIDENCE;

    *b_out = log10(exp(1.0)) / denominator;
    return CAMEFF_AVAILABLE;
}



static double neg_loglik_z(
    const double *y,
    const double *g_history,
    size_t n,
    const double z[2],
    double grad[2]
) {
    double mu = exp(z[0]);
    double k = exp(z[1]);
    double value = 0.0;
    double d_mu = 0.0;
    double d_k = 0.0;
    size_t i;

    for (i = 0; i < n; ++i) {
        double lambda = mu + k * g_history[i];
        double residual;
        if (lambda < 1e-12) lambda = 1e-12;
        value += lambda - y[i] * log(lambda);
        residual = 1.0 - y[i] / lambda;
        d_mu += residual * mu;
        d_k += residual * k * g_history[i];
    }

    grad[0] = d_mu;
    grad[1] = d_k;
    return value;
}

static void optimize_mu_k(
    const double *y,
    const double *g_history,
    size_t n,
    double *mu_out,
    double *k_out,
    double *ll_out
) {
    const int dimension = 2;
    const int memory = 10;
    const int max_iterations = 2000;
    const int max_line_search = 20;
    const double ftol = 1e-12;
    const double machine_epsilon = 2.220446049250313080847263336181640625e-16;
    const double factr = ftol / machine_epsilon;
    const double pgtol = 1e-8;

    double mean_y = 0.0;
    double mean_g = 0.0;
    double x[2];
    double lower[2] = {-15.0, -20.0};
    double upper[2] = {5.0, 5.0};
    int nbd[2] = {2, 2};
    double f = 0.0;
    double gradient[2] = {0.0, 0.0};
    int task[2] = {0, 0};
    int ln_task[2] = {0, 0};
    int lsave[4] = {0, 0, 0, 0};
    int isave[44] = {0};
    double dsave[29] = {0.0};
    int iterations = 0;
    size_t workspace_size;
    double *wa;
    int *iwa;
    size_t i;

    for (i = 0; i < n; ++i) {
        mean_y += y[i];
        mean_g += g_history[i];
    }
    mean_y /= (double)n;
    mean_g /= (double)n;

    x[0] = log(fmax(mean_y * 0.5, 1e-5));
    x[1] = log(fmax(mean_y / (mean_g + 1e-9) * 0.5, 1e-8));

    workspace_size = (size_t)(2 * memory * dimension
        + 5 * dimension + 11 * memory * memory + 8 * memory);
    wa = (double *)calloc(workspace_size, sizeof(double));
    iwa = (int *)calloc((size_t)(3 * dimension), sizeof(int));
    if (wa == NULL || iwa == NULL) {
        free(wa);
        free(iwa);
        *mu_out = exp(x[0]);
        *k_out = exp(x[1]);
        *ll_out = -INFINITY;
        return;
    }

    while (1) {
        cameff_lbfgsb_setulb(
            dimension, memory, x, lower, upper, nbd, &f, gradient,
            factr, pgtol, wa, iwa, task, lsave, isave, dsave,
            max_line_search, ln_task
        );

        if (task[0] == 3) {
            f = neg_loglik_z(y, g_history, n, x, gradient);
        } else if (task[0] == 1) {
            iterations++;
            if (iterations >= max_iterations) {
                task[0] = 5;
                task[1] = 504;
            }
        } else {
            break;
        }
    }

    *mu_out = exp(x[0]);
    *k_out = exp(x[1]);
    *ll_out = -f;

    free(wa);
    free(iwa);
}

cameff_availability_t cameff_fit_temporal_etas(
    const cameff_event_t *events,
    size_t count,
    double decision_time_days,
    cameff_etas_fit_t *fit_out
) {
    double mc, b_value;
    size_t above_count = 0, total_count = 0, i;
    long start_day, end_day;
    size_t days;
    double *y = NULL, *marks = NULL, *g = NULL;
    cameff_etas_fit_t best;
    int have_best = 0;

    if (fit_out == NULL || events == NULL) return CAMEFF_INSUFFICIENT_EVIDENCE;
    memset(fit_out, 0, sizeof(*fit_out));
    fit_out->status = CAMEFF_INSUFFICIENT_EVIDENCE;

    if (cameff_estimate_mc(events, count, decision_time_days, &mc) != CAMEFF_AVAILABLE)
        return CAMEFF_INSUFFICIENT_EVIDENCE;
    if (cameff_estimate_b_value(events, count, decision_time_days, mc, &b_value, &above_count)
        != CAMEFF_AVAILABLE)
        return CAMEFF_INSUFFICIENT_EVIDENCE;

    start_day = LONG_MAX;
    end_day = (long)floor(decision_time_days - 1e-12);
    for (i = 0; i < count; ++i) {
        if (events[i].time_days >= decision_time_days) continue;
        if (!isfinite(events[i].magnitude)) continue;
        {
            long day = (long)floor(events[i].time_days);
            if (day < start_day) start_day = day;
        }
        total_count++;
    }
    if (start_day == LONG_MAX || end_day < start_day)
        return CAMEFF_INSUFFICIENT_EVIDENCE;

    days = (size_t)(end_day - start_day + 1);
    y = (double *)calloc(days, sizeof(double));
    marks = (double *)calloc(days, sizeof(double));
    g = (double *)calloc(days, sizeof(double));
    if (y == NULL || marks == NULL || g == NULL) goto cleanup;

    for (i = 0; i < count; ++i) {
        long index;
        if (events[i].time_days >= decision_time_days) continue;
        if (events[i].magnitude < mc) continue;
        index = (long)floor(events[i].time_days) - start_day;
        if (index < 0 || (size_t)index >= days) continue;
        y[index] += 1.0;
    }

    memset(&best, 0, sizeof(best));
    best.loglik = -DBL_MAX;

    {
        size_t ai, ci, pi;
        for (ai = 0; ai < sizeof(CAMEFF_ALPHA_GRID)/sizeof(CAMEFF_ALPHA_GRID[0]); ++ai) {
            double alpha = CAMEFF_ALPHA_GRID[ai];
            memset(marks, 0, days * sizeof(double));

            for (i = 0; i < count; ++i) {
                long index;
                if (events[i].time_days >= decision_time_days) continue;
                if (events[i].magnitude < mc) continue;
                index = (long)floor(events[i].time_days) - start_day;
                if (index < 0 || (size_t)index >= days) continue;
                marks[index] += exp(alpha * (events[i].magnitude - mc));
            }

            for (ci = 0; ci < sizeof(CAMEFF_C_GRID)/sizeof(CAMEFF_C_GRID[0]); ++ci) {
                for (pi = 0; pi < sizeof(CAMEFF_P_GRID)/sizeof(CAMEFF_P_GRID[0]); ++pi) {
                    double c = CAMEFF_C_GRID[ci];
                    double p = CAMEFF_P_GRID[pi];
                    double mu, k, ll;
                    double kernel_sum = 0.0;
                    double mark_sum = 0.0;
                    size_t mark_n = 0;
                    size_t t, lag;

                    memset(g, 0, days * sizeof(double));
                    for (lag = 1; lag <= CAMEFF_TRIGGER_MEMORY_DAYS; ++lag) {
                        kernel_sum += pow((double)lag + c, -p);
                    }

                    for (t = 0; t < days; ++t) {
                        size_t max_lag = t < CAMEFF_TRIGGER_MEMORY_DAYS ? t : CAMEFF_TRIGGER_MEMORY_DAYS;
                        double total = 0.0;
                        for (lag = 1; lag <= max_lag; ++lag) {
                            total += marks[t - lag] * pow((double)lag + c, -p);
                        }
                        g[t] = total;
                    }

                    for (i = 0; i < count; ++i) {
                        if (events[i].time_days >= decision_time_days) continue;
                        if (events[i].magnitude < mc) continue;
                        mark_sum += exp(alpha * (events[i].magnitude - mc));
                        mark_n++;
                    }

                    optimize_mu_k(y, g, days, &mu, &k, &ll);
                    {
                        double mean_mark = mark_n > 0 ? mark_sum / (double)mark_n : 0.0;
                        double branching = k * kernel_sum * mean_mark;
                        if (branching < 1.0 && ll > best.loglik) {
                            best.status = CAMEFF_AVAILABLE;
                            best.mc = mc;
                            best.b_value = b_value;
                            best.alpha = alpha;
                            best.c = c;
                            best.p = p;
                            best.mu = mu;
                            best.k = k;
                            best.loglik = ll;
                            best.branching_proxy = branching;
                            best.events_total = total_count;
                            best.events_above_mc = above_count;
                            have_best = 1;
                        }
                    }
                }
            }
        }
    }

cleanup:
    free(y);
    free(marks);
    free(g);

    if (!have_best) return CAMEFF_UNKNOWN_PATTERN;
    *fit_out = best;
    return CAMEFF_AVAILABLE;
}

cameff_availability_t cameff_seven_day_m6_probability(
    const cameff_event_t *events,
    size_t count,
    double decision_time_days,
    const cameff_etas_fit_t *fit,
    double *probability_out
) {
    long start_day, decision_day;
    size_t days, i, t, lag;
    double *marks = NULL, *g = NULL;
    double daily_rate, m6_tail, probability;

    if (probability_out != NULL) *probability_out = 0.0;
    if (events == NULL || fit == NULL || probability_out == NULL)
        return CAMEFF_INSUFFICIENT_EVIDENCE;
    if (fit->status != CAMEFF_AVAILABLE)
        return CAMEFF_INSUFFICIENT_EVIDENCE;

    start_day = LONG_MAX;
    decision_day = (long)floor(decision_time_days);
    for (i = 0; i < count; ++i) {
        if (events[i].time_days >= decision_time_days) continue;
        if ((long)floor(events[i].time_days) < start_day)
            start_day = (long)floor(events[i].time_days);
    }
    if (start_day == LONG_MAX || decision_day < start_day)
        return CAMEFF_INSUFFICIENT_EVIDENCE;

    days = (size_t)(decision_day - start_day + 1);
    marks = (double *)calloc(days, sizeof(double));
    g = (double *)calloc(days, sizeof(double));
    if (marks == NULL || g == NULL) goto fail;

    for (i = 0; i < count; ++i) {
        long index;
        if (events[i].time_days >= decision_time_days) continue;
        if (events[i].magnitude < fit->mc) continue;
        index = (long)floor(events[i].time_days) - start_day;
        if (index < 0 || (size_t)index >= days) continue;
        marks[index] += exp(fit->alpha * (events[i].magnitude - fit->mc));
    }

    for (t = 0; t < days; ++t) {
        size_t max_lag = t < CAMEFF_TRIGGER_MEMORY_DAYS ? t : CAMEFF_TRIGGER_MEMORY_DAYS;
        double total = 0.0;
        for (lag = 1; lag <= max_lag; ++lag) {
            total += marks[t - lag] * pow((double)lag + fit->c, -fit->p);
        }
        g[t] = total;
    }

    daily_rate = fmax(fit->mu + fit->k * g[days - 1], 1e-12);
    m6_tail = pow(10.0, -fit->b_value * (6.0 - fit->mc));
    if (m6_tail > 1.0) m6_tail = 1.0;
    probability = 1.0 - exp(-7.0 * daily_rate * m6_tail);
    if (probability < 1e-9) probability = 1e-9;
    if (probability > 1.0 - 1e-9) probability = 1.0 - 1e-9;

    free(marks);
    free(g);
    *probability_out = probability;
    return CAMEFF_AVAILABLE;

fail:
    free(marks);
    free(g);
    return CAMEFF_INSUFFICIENT_EVIDENCE;
}

double cameff_beta_probability(
    double raw_probability,
    const cameff_calibration_t *calibration
) {
    double p, z;
    if (calibration == NULL || calibration->status != CAMEFF_AVAILABLE) return NAN;
    p = raw_probability;
    if (p < 1e-9) p = 1e-9;
    if (p > 1.0 - 1e-9) p = 1.0 - 1e-9;
    z = calibration->intercept
      + calibration->coef_log_p * log(p)
      + calibration->coef_neg_log_1mp * (-log(1.0 - p));
    return cameff_sigmoid(z);
}

cameff_availability_t cameff_apply_p29k_calibration(
    double raw_probability,
    const cameff_calibration_t *calibration,
    double *calibrated_out
) {
    double beta, final_probability;
    if (calibrated_out != NULL) *calibrated_out = 0.0;
    if (calibration == NULL || calibrated_out == NULL)
        return CAMEFF_INSUFFICIENT_EVIDENCE;
    if (calibration->status != CAMEFF_AVAILABLE)
        return calibration->status;
    if (calibration->training_rows < 500 || calibration->training_positives < 10)
        return CAMEFF_INSUFFICIENT_EVIDENCE;

    beta = cameff_beta_probability(raw_probability, calibration);
    if (!isfinite(beta)) return CAMEFF_INSUFFICIENT_EVIDENCE;

    final_probability = 0.5 * beta + 0.5 * calibration->training_prevalence;
    if (final_probability < 1e-9) final_probability = 1e-9;
    if (final_probability > 1.0 - 1e-9) final_probability = 1.0 - 1e-9;
    *calibrated_out = final_probability;
    return CAMEFF_AVAILABLE;
}

const char *cameff_availability_name(cameff_availability_t status) {
    switch (status) {
        case CAMEFF_AVAILABLE: return "AVAILABLE";
        case CAMEFF_UNAVAILABLE_PARAMETER: return "UNAVAILABLE_PARAMETER";
        case CAMEFF_INSUFFICIENT_EVIDENCE: return "INSUFFICIENT_EVIDENCE";
        case CAMEFF_INELIGIBLE_TEMPORAL_MODE: return "INELIGIBLE_TEMPORAL_MODE";
        case CAMEFF_UNKNOWN_PATTERN: return "UNKNOWN_PATTERN";
        default: return "UNKNOWN_STATUS";
    }
}
