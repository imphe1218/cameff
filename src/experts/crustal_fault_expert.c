#include "cameff/crustal_fault_expert.h"

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

static double crustal_applicability(
    const cameff_signal_frame_t *frame,
    const cameff_region_profile_t *region,
    const cameff_pattern_result_t *patterns
) {
    double mapped_affinity;
    double uncertainty_support;

    (void)frame;
    (void)patterns;

    if (region == NULL) {
        return 0.0;
    }

    mapped_affinity =
        clamp01(region->crustal_fault_affinity);

    uncertainty_support =
        1.0 - clamp01(region->fault_map_confidence);

    return clamp01(
        (0.75 * mapped_affinity) +
        (0.25 * uncertainty_support)
    );
}

static cameff_status_t crustal_evaluate(
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
    double b_value;
    double fault_confidence;
    double uncertainty;
    double observational_support;
    double uncertainty_modifier;
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

    b_value = effective_signal(
        frame,
        CAMEFF_SIGNAL_B_VALUE_ANOMALY
    );

    fault_confidence = effective_signal(
        frame,
        CAMEFF_SIGNAL_FAULT_MAP_CONFIDENCE
    );

    uncertainty =
        1.0 - clamp01(region->fault_map_confidence);

    observational_support =
        (0.30 * activity) +
        (0.25 * acceleration) +
        (0.25 * concentration) +
        (0.12 * magnitude) +
        (0.08 * b_value);

    /*
     * Fault-map uncertainty can increase compatibility only when
     * observed activity and spatial concentration are present.
     */
    uncertainty_modifier =
        uncertainty * activity * concentration;

    preparation =
        observational_support +
        (0.15 * uncertainty_modifier);

    hazard =
        (0.75 * preparation) +
        (0.15 * clamp01(region->crustal_fault_affinity)) +
        (0.10 * fault_confidence);

    confidence =
        clamp01(
            (0.60 * frame->data_confidence) +
            (0.20 * region->catalog_completeness) +
            (0.15 * region->station_coverage) +
            (0.05 * region->regional_prior_confidence)
        );

    (void)snprintf(
        result->expert_name,
        sizeof(result->expert_name),
        "%s",
        "crustal-fault"
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
            "Crustal-fault interpretation limited by low catalog or station confidence."
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
            "Crustal-fault evidence uses activity, acceleration, concentration and guarded fault-map uncertainty."
        );
    }

    return CAMEFF_STATUS_OK;
}

const cameff_expert_t *cameff_crustal_fault_expert(void) {
    static const cameff_expert_t expert = {
        "crustal-fault",
        CAMEFF_PATTERN_CRUSTAL_FAULT_ACTIVATION,
        crustal_applicability,
        crustal_evaluate
    };

    return &expert;
}