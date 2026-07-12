#include "cameff/cascade_expert.h"

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

static double cascade_applicability(
    const cameff_signal_frame_t *frame,
    const cameff_region_profile_t *region,
    const cameff_pattern_result_t *patterns
) {
    double activity;
    double concentration;

    (void)region;
    (void)patterns;

    if (frame == NULL) {
        return 0.0;
    }

    activity = effective_signal(
        frame,
        CAMEFF_SIGNAL_ACTIVITY_RATE
    );

    concentration = effective_signal(
        frame,
        CAMEFF_SIGNAL_SPATIAL_CONCENTRATION
    );

    return clamp01(
        (0.55 * activity) +
        (0.45 * concentration)
    );
}

static cameff_status_t cascade_evaluate(
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
    double completeness;
    double cascade;
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

    completeness = effective_signal(
        frame,
        CAMEFF_SIGNAL_CATALOG_COMPLETENESS
    );

    cascade =
        (0.35 * activity) +
        (0.20 * acceleration) +
        (0.30 * concentration) +
        (0.05 * magnitude) +
        (0.10 * completeness);

    /*
     * A cascade interpretation competes with preparation.
     * High cascade compatibility reduces preparation evidence.
     */
    preparation =
        clamp01(
            (0.40 * acceleration) +
            (0.35 * magnitude) +
            (0.25 * concentration) -
            (0.45 * cascade)
        );

    hazard =
        clamp01(
            (0.65 * cascade) +
            (0.25 * activity) +
            (0.10 * magnitude)
        );

    confidence =
        clamp01(
            (0.65 * frame->data_confidence) +
            (0.20 * region->catalog_completeness) +
            (0.15 * region->station_coverage)
        );

    (void)snprintf(
        result->expert_name,
        sizeof(result->expert_name),
        "%s",
        "seismic-cascade"
    );

    result->hazard_evidence = hazard;
    result->preparation_evidence = preparation;
    result->cascade_evidence = clamp01(cascade);
    result->confidence = confidence;

    if (confidence < 0.25) {
        result->status = CAMEFF_EXPERT_INSUFFICIENT_DATA;
        result->decision = CAMEFF_DECISION_INSUFFICIENT_DATA;

        (void)snprintf(
            result->explanation,
            sizeof(result->explanation),
            "%s",
            "Cascade interpretation limited by low catalog confidence."
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
            "Cascade evidence emphasizes elevated activity and spatial clustering while suppressing unsupported preparation evidence."
        );
    }

    return CAMEFF_STATUS_OK;
}

const cameff_expert_t *cameff_cascade_expert(void) {
    static const cameff_expert_t expert = {
        "seismic-cascade",
        CAMEFF_PATTERN_SEISMIC_CASCADE,
        cascade_applicability,
        cascade_evaluate
    };

    return &expert;
}