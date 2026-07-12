#include "cameff/pattern.h"

#include <float.h>
#include <math.h>
#include <string.h>

#define CAMEFF_CLASSIFIER_EPSILON 1e-12

static double clamp01(double value) {
    if (!isfinite(value)) {
        return 0.0;
    }

    if (value < 0.0) {
        return 0.0;
    }

    if (value > 1.0) {
        return 1.0;
    }

    return value;
}

static double signal_value(
    const cameff_signal_frame_t *frame,
    cameff_signal_id_t signal_id
) {
    const cameff_signal_t *signal;

    signal = &frame->signals[(size_t)signal_id];

    if (!signal->available) {
        return 0.0;
    }

    return clamp01(signal->value);
}

static double signal_confidence(
    const cameff_signal_frame_t *frame,
    cameff_signal_id_t signal_id
) {
    const cameff_signal_t *signal;

    signal = &frame->signals[(size_t)signal_id];

    if (!signal->available) {
        return 0.0;
    }

    return clamp01(signal->confidence);
}

static double effective_signal(
    const cameff_signal_frame_t *frame,
    cameff_signal_id_t signal_id
) {
    return signal_value(frame, signal_id) *
           signal_confidence(frame, signal_id);
}

static double mean_available_signal_confidence(
    const cameff_signal_frame_t *frame
) {
    double total;
    size_t available_count;
    size_t i;

    total = 0.0;
    available_count = 0U;

    for (i = 0U; i < CAMEFF_SIGNAL_COUNT; ++i) {
        if (frame->signals[i].available) {
            total += clamp01(frame->signals[i].confidence);
            ++available_count;
        }
    }

    if (available_count == 0U) {
        return 0.0;
    }

    return total / (double)available_count;
}

static double region_profile_confidence(
    const cameff_region_profile_t *region
) {
    double total;

    total =
        clamp01(region->catalog_completeness) +
        clamp01(region->station_coverage) +
        clamp01(region->regional_prior_confidence);

    return total / 3.0;
}

static void initialize_pattern_result(
    cameff_pattern_result_t *result
) {
    size_t i;

    (void)memset(result, 0, sizeof(*result));

    result->count = CAMEFF_PATTERN_COUNT;
    result->dominant_pattern = CAMEFF_PATTERN_UNKNOWN;

    for (i = 0U; i < CAMEFF_PATTERN_COUNT; ++i) {
        result->scores[i].pattern_id = (cameff_pattern_id_t)i;
    }
}

static void calculate_raw_scores(
    const cameff_signal_frame_t *frame,
    const cameff_region_profile_t *region,
    double scores[CAMEFF_PATTERN_COUNT]
) {
    double activity;
    double acceleration;
    double concentration;
    double magnitude;
    double depth;
    double b_value;
    double completeness;
    double fault_confidence;

    double subduction;
    double crustal;
    double transform;
    double volcanic;
    double fault_uncertainty;
    double data_confidence;
    double quietness;

    activity = effective_signal(
        frame,
        CAMEFF_SIGNAL_ACTIVITY_RATE
    );

    acceleration = effective_signal(
        frame,
        CAMEFF_SIGNAL_TEMPORAL_ACCELERATION
    );

    concentration = effective_signal(
        frame,
        CAMEFF_SIGNAL_SPATIAL_CONCENTRATION
    );

    magnitude = effective_signal(
        frame,
        CAMEFF_SIGNAL_MAGNITUDE_TREND
    );

    depth = effective_signal(
        frame,
        CAMEFF_SIGNAL_DEPTH_MIGRATION
    );

    b_value = effective_signal(
        frame,
        CAMEFF_SIGNAL_B_VALUE_ANOMALY
    );

    completeness = effective_signal(
        frame,
        CAMEFF_SIGNAL_CATALOG_COMPLETENESS
    );

    fault_confidence = effective_signal(
        frame,
        CAMEFF_SIGNAL_FAULT_MAP_CONFIDENCE
    );

    subduction = clamp01(region->subduction_affinity);
    crustal = clamp01(region->crustal_fault_affinity);
    transform = clamp01(region->transform_affinity);
    volcanic = clamp01(region->volcanic_affinity);

    fault_uncertainty =
        1.0 - clamp01(region->fault_map_confidence);

    data_confidence =
        clamp01(frame->data_confidence);

    quietness =
        1.0 -
        (
            activity +
            acceleration +
            concentration +
            magnitude +
            depth +
            b_value
        ) / 6.0;

    /*
     * Background compatibility rises when anomaly channels are quiet
     * and the input data are reasonably trustworthy.
     */
    scores[CAMEFF_PATTERN_BACKGROUND] =
        0.20 +
        (1.80 * quietness) +
        (0.40 * completeness) +
        (0.30 * data_confidence);

    /*
     * Subduction preparation emphasizes temporal acceleration,
     * magnitude behavior, spatial concentration and depth behavior,
     * conditioned on subduction-region applicability.
     */
    scores[CAMEFF_PATTERN_SUBDUCTION_PREPARATION] =
        -0.30 +
        (1.30 * subduction) +
        (0.75 * activity) +
        (1.00 * acceleration) +
        (0.65 * concentration) +
        (0.85 * magnitude) +
        (0.65 * depth) +
        (0.50 * b_value) +
        (0.25 * completeness);

    /*
     * Crustal-fault activation uses concentrated shallow-style
     * catalog evidence and permits fault-map uncertainty to increase
     * applicability. Uncertainty alone is insufficient because it is
     * multiplied by observed activity and concentration.
     */
    scores[CAMEFF_PATTERN_CRUSTAL_FAULT_ACTIVATION] =
        -0.35 +
        (1.20 * crustal) +
        (0.85 * activity) +
        (0.80 * acceleration) +
        (1.05 * concentration) +
        (0.65 * magnitude) +
        (0.30 * b_value) +
        (0.55 * fault_uncertainty * activity * concentration) +
        (0.20 * fault_confidence);

    /*
     * Cascade compatibility emphasizes high activity and clustering.
     * It deliberately uses less tectonic-context dependence because
     * triggered sequences can occur in several tectonic settings.
     */
    scores[CAMEFF_PATTERN_SEISMIC_CASCADE] =
        -0.20 +
        (1.25 * activity) +
        (0.75 * acceleration) +
        (1.20 * concentration) +
        (0.35 * magnitude) +
        (0.20 * b_value) +
        (0.20 * completeness);

    /*
     * Volcanic classification remains conservative because Milestone 1
     * does not yet provide deformation, gas or thermal observations.
     */
    scores[CAMEFF_PATTERN_VOLCANIC_UNREST] =
        -0.75 +
        (1.45 * volcanic) +
        (0.70 * activity) +
        (0.70 * acceleration) +
        (0.55 * concentration) +
        (0.55 * depth);

    /*
     * Transform-fault activity emphasizes lateral concentration,
     * temporal acceleration and regional transform affinity.
     */
    scores[CAMEFF_PATTERN_TRANSFORM_FAULT_ACTIVITY] =
        -0.45 +
        (1.35 * transform) +
        (0.80 * activity) +
        (0.85 * acceleration) +
        (0.95 * concentration) +
        (0.55 * magnitude) +
        (0.30 * b_value);

    /*
     * Unknown becomes more compatible when data quality or regional
     * knowledge is weak.
     */
    scores[CAMEFF_PATTERN_UNKNOWN] =
        -0.10 +
        (0.90 * (1.0 - data_confidence)) +
        (0.65 * (1.0 - completeness)) +
        (0.55 * (1.0 - region_profile_confidence(region)));
}

static void softmax_scores(
    const double raw_scores[CAMEFF_PATTERN_COUNT],
    cameff_pattern_result_t *result
) {
    double maximum;
    double denominator;
    double exponentials[CAMEFF_PATTERN_COUNT];
    size_t dominant_index;
    size_t i;

    maximum = -DBL_MAX;

    for (i = 0U; i < CAMEFF_PATTERN_COUNT; ++i) {
        if (raw_scores[i] > maximum) {
            maximum = raw_scores[i];
        }
    }

    denominator = 0.0;

    for (i = 0U; i < CAMEFF_PATTERN_COUNT; ++i) {
        exponentials[i] = exp(raw_scores[i] - maximum);
        denominator += exponentials[i];
    }

    if (!isfinite(denominator) ||
        denominator <= CAMEFF_CLASSIFIER_EPSILON) {
        result->scores[CAMEFF_PATTERN_UNKNOWN].membership = 1.0;
        result->dominant_pattern = CAMEFF_PATTERN_UNKNOWN;
        return;
    }

    dominant_index = 0U;

    for (i = 0U; i < CAMEFF_PATTERN_COUNT; ++i) {
        result->scores[i].membership =
            exponentials[i] / denominator;

        if (result->scores[i].membership >
            result->scores[dominant_index].membership) {
            dominant_index = i;
        }
    }

    result->dominant_pattern =
        (cameff_pattern_id_t)dominant_index;
}

cameff_status_t cameff_classify_patterns(
    const cameff_signal_frame_t *frame,
    const cameff_region_profile_t *region,
    cameff_pattern_result_t *result
) {
    double raw_scores[CAMEFF_PATTERN_COUNT];
    double signal_quality;
    double regional_quality;
    size_t i;

    if (frame == NULL || region == NULL || result == NULL) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }

    initialize_pattern_result(result);
    calculate_raw_scores(frame, region, raw_scores);
    softmax_scores(raw_scores, result);

    signal_quality =
        mean_available_signal_confidence(frame);

    regional_quality =
        region_profile_confidence(region);

    result->classification_confidence =
        clamp01(
            (0.70 * signal_quality) +
            (0.30 * regional_quality)
        );

    for (i = 0U; i < CAMEFF_PATTERN_COUNT; ++i) {
        result->scores[i].confidence =
            result->classification_confidence;
    }

    return CAMEFF_STATUS_OK;
}

const char *cameff_pattern_name(cameff_pattern_id_t pattern_id) {
    switch (pattern_id) {
        case CAMEFF_PATTERN_BACKGROUND:
            return "BACKGROUND";

        case CAMEFF_PATTERN_SUBDUCTION_PREPARATION:
            return "SUBDUCTION_PREPARATION";

        case CAMEFF_PATTERN_CRUSTAL_FAULT_ACTIVATION:
            return "CRUSTAL_FAULT_ACTIVATION";

        case CAMEFF_PATTERN_SEISMIC_CASCADE:
            return "SEISMIC_CASCADE";

        case CAMEFF_PATTERN_VOLCANIC_UNREST:
            return "VOLCANIC_UNREST";

        case CAMEFF_PATTERN_TRANSFORM_FAULT_ACTIVITY:
            return "TRANSFORM_FAULT_ACTIVITY";

        case CAMEFF_PATTERN_UNKNOWN:
            return "UNKNOWN";

        default:
            return "INVALID_PATTERN";
    }
}