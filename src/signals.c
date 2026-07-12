#include "cameff/signals.h"

#include "cameff/geo.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static double clamp01(double value) {
    return fmin(1.0, fmax(0.0, value));
}

const char *cameff_signal_name(cameff_signal_id_t signal_id) {
    static const char *names[CAMEFF_SIGNAL_COUNT] = {
        "activity_rate", "temporal_acceleration", "spatial_concentration", "magnitude_trend",
        "depth_migration", "b_value_anomaly", "catalog_completeness", "fault_map_confidence"
    };
    if ((unsigned)signal_id >= CAMEFF_SIGNAL_COUNT) {
        return "unknown";
    }
    return names[(unsigned)signal_id];
}

static int compare_epoch(const void *left, const void *right) {
    const cameff_event_t *a = left;
    const cameff_event_t *b = right;
    return (a->epoch_seconds > b->epoch_seconds) - (a->epoch_seconds < b->epoch_seconds);
}

static double mean_magnitude(const cameff_event_t *events, size_t begin, size_t end) {
    size_t i;
    double sum = 0.0;
    if (end <= begin) return 0.0;
    for (i = begin; i < end; i++) sum += events[i].magnitude;
    return sum / (double)(end - begin);
}

cameff_status_t cameff_extract_signal_frame(const cameff_catalog_t *catalog,
                                            const cameff_analysis_window_t *window,
                                            const cameff_config_t *config,
                                            cameff_signal_frame_t *frame) {
    cameff_event_t *selected;
    size_t i;
    size_t count = 0U;
    const int64_t start_epoch = window->cutoff_epoch_seconds -
                                ((int64_t)window->lookback_days * 86400);
    double mean_distance = 0.0;
    double mean_depth_first = 0.0;
    double mean_depth_second = 0.0;
    size_t recent_count = 0U;
    size_t older_count = 0U;
    size_t first_count;
    size_t second_count;
    double min_magnitude = 100.0;
    double magnitude_sum = 0.0;
    double magnitude_sq_sum = 0.0;

    if ((catalog == NULL) || (window == NULL) || (config == NULL) || (frame == NULL) ||
        (window->radius_km <= 0.0) || (window->lookback_days == 0U)) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }
    (void)memset(frame, 0, sizeof(*frame));
    selected = malloc((catalog->count == 0U ? 1U : catalog->count) * sizeof(*selected));
    if (selected == NULL) return CAMEFF_STATUS_OUT_OF_MEMORY;

    for (i = 0U; i < catalog->count; i++) {
        const cameff_event_t *event = &catalog->items[i];
        const double distance = cameff_haversine_km(window->center_latitude_deg,
                                                   window->center_longitude_deg,
                                                   event->latitude_deg, event->longitude_deg);
        if ((event->epoch_seconds >= start_epoch) &&
            (event->epoch_seconds <= window->cutoff_epoch_seconds) &&
            (distance <= window->radius_km)) {
            selected[count++] = *event;
        }
    }
    frame->selected_event_count = count;
    if (count == 0U) {
        free(selected);
        return CAMEFF_STATUS_INSUFFICIENT_DATA;
    }
    qsort(selected, count, sizeof(*selected), compare_epoch);
    first_count = count / 2U;
    second_count = count - first_count;

    for (i = 0U; i < count; i++) {
        const double distance = cameff_haversine_km(window->center_latitude_deg,
                                                   window->center_longitude_deg,
                                                   selected[i].latitude_deg,
                                                   selected[i].longitude_deg);
        mean_distance += distance;
        magnitude_sum += selected[i].magnitude;
        magnitude_sq_sum += selected[i].magnitude * selected[i].magnitude;
        if (selected[i].magnitude < min_magnitude) min_magnitude = selected[i].magnitude;
        if (i < first_count) mean_depth_first += selected[i].depth_km;
        else mean_depth_second += selected[i].depth_km;
        if (selected[i].epoch_seconds >= window->cutoff_epoch_seconds -
                                               ((int64_t)window->lookback_days * 43200)) recent_count++;
        else older_count++;
    }
    mean_distance /= (double)count;
    if (first_count > 0U) mean_depth_first /= (double)first_count;
    if (second_count > 0U) mean_depth_second /= (double)second_count;

    frame->signals[CAMEFF_SIGNAL_ACTIVITY_RATE] = (cameff_signal_t){
        clamp01((double)count / ((double)config->minimum_events * 2.0)),
        clamp01((double)count / (double)config->minimum_events), count, 1
    };
    frame->signals[CAMEFF_SIGNAL_TEMPORAL_ACCELERATION] = (cameff_signal_t){
        clamp01(0.5 + (((double)recent_count - (double)older_count) / (double)count)),
        clamp01((double)count / ((double)config->minimum_events * 1.5)), count, 1
    };
    frame->signals[CAMEFF_SIGNAL_SPATIAL_CONCENTRATION] = (cameff_signal_t){
        clamp01(1.0 - (mean_distance / window->radius_km)),
        clamp01((double)count / (double)config->minimum_events), count, 1
    };
    frame->signals[CAMEFF_SIGNAL_MAGNITUDE_TREND] = (cameff_signal_t){
        clamp01(0.5 + (mean_magnitude(selected, first_count, count) -
                       mean_magnitude(selected, 0U, first_count)) / 2.0),
        clamp01((double)count / ((double)config->minimum_events * 1.5)), count, first_count > 0U
    };
    frame->signals[CAMEFF_SIGNAL_DEPTH_MIGRATION] = (cameff_signal_t){
        clamp01(0.5 + (mean_depth_first - mean_depth_second) / 100.0),
        clamp01((double)count / ((double)config->minimum_events * 1.5)), count, first_count > 0U
    };

    {
        const double mean_mag = magnitude_sum / (double)count;
        const double variance = fmax(0.0, magnitude_sq_sum / (double)count - mean_mag * mean_mag);
        const double b_proxy = (mean_mag > min_magnitude) ?
                               (0.4342944819 / (mean_mag - min_magnitude + 0.05)) : 2.0;
        frame->signals[CAMEFF_SIGNAL_B_VALUE_ANOMALY] = (cameff_signal_t){
            clamp01((1.2 - b_proxy) / 0.8),
            clamp01((double)count / 20.0) * clamp01(variance / 0.25), count, count >= 10U
        };
    }
    frame->signals[CAMEFF_SIGNAL_CATALOG_COMPLETENESS] = (cameff_signal_t){
        clamp01((double)count / ((double)config->minimum_events * 2.0)),
        clamp01((double)count / (double)config->minimum_events), count, 1
    };
    frame->signals[CAMEFF_SIGNAL_FAULT_MAP_CONFIDENCE] = (cameff_signal_t){
        clamp01(config->fault_map_confidence), 1.0, 0U, 1
    };

    {
        double confidence_sum = 0.0;
        unsigned available_count = 0U;
        for (i = 0U; i < CAMEFF_SIGNAL_COUNT; i++) {
            if (frame->signals[i].available != 0) {
                confidence_sum += frame->signals[i].confidence;
                available_count++;
            }
        }
        frame->data_confidence = available_count > 0U ? confidence_sum / (double)available_count : 0.0;
    }
    free(selected);
    return count >= config->minimum_events ? CAMEFF_STATUS_OK : CAMEFF_STATUS_INSUFFICIENT_DATA;
}
