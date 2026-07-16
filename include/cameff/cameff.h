#ifndef CAMEFF_H
#define CAMEFF_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAMEFF_MAX_EXPERTS 15
#define CAMEFF_MAX_SIGNALS 64
#define CAMEFF_MAX_NAME 64
#define CAMEFF_EPS 1e-12

typedef enum {
    CAMEFF_AVAILABLE = 0,
    CAMEFF_UNAVAILABLE_PARAMETER = 1,
    CAMEFF_INSUFFICIENT_EVIDENCE = 2,
    CAMEFF_INELIGIBLE_TEMPORAL_MODE = 3,
    CAMEFF_UNKNOWN_PATTERN = 4
} cameff_availability_t;

typedef enum {
    CAMEFF_MODE_A_GENERAL_MOE = 0,
    CAMEFF_MODE_B_CATALOG_MULTISIGNAL = 1,
    CAMEFF_MODE_C_LONG_TERM_IMMINENCE = 2,
    CAMEFF_MODE_D_POST_TRIGGER = 3
} cameff_processing_mode_t;

typedef struct {
    double time_days;
    double magnitude;
} cameff_event_t;

typedef struct {
    const char *name;
    double value;
    double reliability;
    int available;
} cameff_signal_t;

typedef struct {
    const char *expert_id;
    cameff_availability_t status;
    double compatibility;
    double routing_weight;
    double output;
    double reliability;
    double effective_weight;
    double contribution;
} cameff_expert_result_t;

typedef struct {
    cameff_availability_t status;
    double mc;
    double b_value;
    double alpha;
    double c;
    double p;
    double mu;
    double k;
    double loglik;
    double branching_proxy;
    size_t events_total;
    size_t events_above_mc;
} cameff_etas_fit_t;

typedef struct {
    cameff_availability_t status;
    size_t training_rows;
    size_t training_positives;
    double training_prevalence;
    double intercept;
    double coef_log_p;
    double coef_neg_log_1mp;
} cameff_calibration_t;

typedef struct {
    cameff_availability_t status;
    double raw_probability;
    double calibrated_probability;
    cameff_etas_fit_t fit;
    cameff_calibration_t calibration;
} cameff_forecast_t;

/* Core numerical utilities */
double cameff_sigmoid(double x);
double cameff_probability_logit(double p);
double cameff_clip01(double x);

/* FM-v1.4 reference architecture utilities */
double cameff_signal_effective(const cameff_signal_t *signal);
double cameff_weighted_quality(
    const cameff_signal_t *signals,
    const double *weights,
    size_t count,
    int *available
);
void cameff_normalize_supports(
    const double *raw,
    size_t count,
    double temperature,
    double *out
);
double cameff_unknown_support(
    double max_known,
    double quality,
    double nearest_domain_distance,
    int has_distance,
    double lambda_max,
    double lambda_quality,
    double lambda_distance,
    double distance_scale
);
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
);

/* Validated operational branch: temporal ETAS + P29K */
cameff_availability_t cameff_estimate_mc(
    const cameff_event_t *events,
    size_t count,
    double decision_time_days,
    double *mc_out
);
cameff_availability_t cameff_estimate_b_value(
    const cameff_event_t *events,
    size_t count,
    double decision_time_days,
    double mc,
    double *b_out,
    size_t *events_above_mc_out
);
cameff_availability_t cameff_fit_temporal_etas(
    const cameff_event_t *events,
    size_t count,
    double decision_time_days,
    cameff_etas_fit_t *fit_out
);
cameff_availability_t cameff_seven_day_m6_probability(
    const cameff_event_t *events,
    size_t count,
    double decision_time_days,
    const cameff_etas_fit_t *fit,
    double *probability_out
);

double cameff_beta_probability(
    double raw_probability,
    const cameff_calibration_t *calibration
);
cameff_availability_t cameff_apply_p29k_calibration(
    double raw_probability,
    const cameff_calibration_t *calibration,
    double *calibrated_out
);

const char *cameff_availability_name(cameff_availability_t status);

#ifdef __cplusplus
}
#endif

#endif
