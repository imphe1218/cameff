#include "cameff/baseline_expert.h"

#include <string.h>

#include "cameff/framework.h"

static double baseline_applicability(
    const cameff_signal_frame_t *frame,
    const cameff_region_profile_t *region,
    const cameff_pattern_result_t *patterns
) {
    (void)region;
    (void)patterns;

    if (frame == NULL) {
        return 0.0;
    }

    return 1.0;
}

static cameff_status_t baseline_evaluate(
    const cameff_signal_frame_t *frame,
    const cameff_config_t *config,
    const cameff_region_profile_t *region,
    const cameff_pattern_result_t *patterns,
    cameff_expert_result_t *result
) {
    cameff_assessment_t assessment;

    (void)region;
    (void)patterns;

    if (frame == NULL || config == NULL || result == NULL) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }

    (void)memset(result, 0, sizeof(*result));

    assessment = cameff_assess(frame, config);

    (void)strncpy(
        result->expert_name,
        "baseline",
        CAMEFF_MAX_EXPERT_NAME - 1U
    );

    result->hazard_evidence = assessment.evidence_score;

    /*
     * Milestone 1 produced a generic evidence score. It did not distinguish
     * preparation from cascade processes.
     */
    result->preparation_evidence = 0.0;
    result->cascade_evidence = 0.0;

    result->confidence = assessment.confidence;
    result->applicability = 1.0;
    result->decision = assessment.level;

    if (assessment.level == CAMEFF_DECISION_INSUFFICIENT_DATA) {
        result->status = CAMEFF_EXPERT_INSUFFICIENT_DATA;

        (void)strncpy(
            result->explanation,
            "Milestone 1 baseline assessment reported insufficient data.",
            CAMEFF_MAX_EXPLANATION - 1U
        );
    } else {
        result->status = CAMEFF_EXPERT_OK;

        (void)strncpy(
            result->explanation,
            "Generic Milestone 1 weighted evidence assessment.",
            CAMEFF_MAX_EXPLANATION - 1U
        );
    }

    return CAMEFF_STATUS_OK;
}

const cameff_expert_t *cameff_baseline_expert(void) {
    static const cameff_expert_t expert = {
        "baseline",
        CAMEFF_PATTERN_UNKNOWN,
        baseline_applicability,
        baseline_evaluate
    };

    return &expert;
}