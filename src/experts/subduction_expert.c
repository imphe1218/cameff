#include "cameff/subduction_expert.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

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

static double effective_signal(
    const cameff_signal_frame_t *frame,
    cameff_signal_id_t signal_id
) {
    const cameff_signal_t *signal;

    signal = &frame->signals[(size_t)signal_id];

    if (!signal->available) {
        return 0.0;
    }

    return clamp01(signal->value) *
           clamp01(signal->confidence);
}

static double subduction_applicability(
    const cameff_signal_frame_t *frame,
    const cameff_region_profile_t *region,
    const cameff_pattern_result_t *patterns
) {
    (void)frame;
    (void)patterns;

    if (region == NULL) {
        return 0.0;
    }

    return clamp01(region->subduction_affinity) *
           clamp01(region->regional_prior_confidence);
}

static cameff_status_t subduction_evaluate(
    const cameff_signal_frame_t *frame,
    const cameff_config_t *config,
    const cameff_region_profile_t *region,
    const cameff_pattern_result_t *patterns,
    cameff_expert_result_t *result
) {
    double activity;
    double acceleration;
    double concentration;
    double magnitude;
    double depth;
    double b_value;
    double completeness;
    double preparation;
    double hazard;
    double confidence;

    (void)config;
    (void)patterns;

    if (frame == NULL || region == NULL || result == NULL) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }

    (void)memset(result, 0, sizeof(*result));

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

    preparation =
        (0.18 * activity) +
        (0.22 * acceleration) +
        (0.15 * concentration) +
        (0.18 * magnitude) +
        (0.15 * depth) +
        (0.12 * b_value);

    hazard =
        (0.70 * preparation) +
        (0.20 * completeness) +
        (0.10 * clamp01(region->subduction_affinity));

    confidence =
        clamp01(
            (0.55 * frame->data_confidence) +
            (0.20 * region->catalog_completeness) +
            (0.15 * region->station_coverage) +
            (0.10 * region->regional_prior_confidence)
        );

    (void)snprintf(
        result->expert_name,
        sizeof(result->expert_name),
        "%s",
        "subduction"
    );

    result->hazard_evidence = clamp01(hazard);
    result->preparation_evidence = clamp01(preparation);
    result->cascade_evidence = 0.0;
    result->confidence = confidence;

    if (confidence < 0.25) {
        result->status = CAMEFF_EXPERT_INSUFFICIENT_DATA;
        result->decision = CAMEFF_DECISION_INSUFFICIENT_DATA;

        (void)snprintf(
            result->explanation,
            sizeof(result->explanation),
            "%s",
            "Subduction interpretation limited by low data confidence."
        );
    } else {
        result->status = CAMEFF_EXPERT_OK;

        if (result->hazard_evidence >= 0.70) {
            result->decision = CAMEFF_DECISION_ELEVATED;
        } else if (result->hazard_evidence >= 0.45) {
            result->decision = CAMEFF_DECISION_WATCH;
        } else {
            result->decision = CAMEFF_DECISION_BACKGROUND;
        }

        (void)snprintf(
            result->explanation,
            sizeof(result->explanation),
            "%s",
            "Subduction preparation evidence derived from activity, acceleration, concentration, magnitude, depth and b-value channels."
        );
    }

    return CAMEFF_STATUS_OK;
}

const cameff_expert_t *cameff_subduction_expert(void) {
    static const cameff_expert_t expert = {
        "subduction",
        CAMEFF_PATTERN_SUBDUCTION_PREPARATION,
        subduction_applicability,
        subduction_evaluate
    };

    return &expert;
}