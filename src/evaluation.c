#include "cameff/evaluation.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "cameff/baseline_expert.h"
#include "cameff/cascade_expert.h"
#include "cameff/crustal_fault_expert.h"
#include "cameff/expert_registry.h"
#include "cameff/fusion.h"
#include "cameff/subduction_expert.h"

static int valid_unit_interval(double value) {
    return isfinite(value) &&
           value >= 0.0 &&
           value <= 1.0;
}

static int valid_options(
    const cameff_evaluation_options_t *options
) {
    if (options == NULL) {
        return 0;
    }

    if (!valid_unit_interval(options->routing_threshold) ||
        !valid_unit_interval(options->minimum_fused_confidence) ||
        !valid_unit_interval(options->watch_threshold) ||
        !valid_unit_interval(options->elevated_threshold)) {
        return 0;
    }

    if (options->watch_threshold >
        options->elevated_threshold) {
        return 0;
    }

    return 1;
}

static const char *expert_status_name(
    cameff_expert_status_t status
) {
    switch (status) {
        case CAMEFF_EXPERT_OK:
            return "OK";

        case CAMEFF_EXPERT_ABSTAIN:
            return "ABSTAIN";

        case CAMEFF_EXPERT_INSUFFICIENT_DATA:
            return "INSUFFICIENT_DATA";

        case CAMEFF_EXPERT_ERROR:
            return "ERROR";

        default:
            return "INVALID_STATUS";
    }
}

static const char *decision_name(
    cameff_decision_level_t decision
) {
    switch (decision) {
        case CAMEFF_DECISION_BACKGROUND:
            return "BACKGROUND";

        case CAMEFF_DECISION_WATCH:
            return "WATCH";

        case CAMEFF_DECISION_ELEVATED:
            return "ELEVATED";

        case CAMEFF_DECISION_INSUFFICIENT_DATA:
            return "INSUFFICIENT_DATA";

        default:
            return "INVALID_DECISION";
    }
}

static void append_text(
    char *buffer,
    size_t capacity,
    size_t *offset,
    const char *format,
    ...
) {
    va_list arguments;
    int written;
    size_t remaining;

    if (buffer == NULL ||
        offset == NULL ||
        format == NULL ||
        capacity == 0U ||
        *offset >= capacity) {
        return;
    }

    remaining = capacity - *offset;

    va_start(arguments, format);

    written = vsnprintf(
        buffer + *offset,
        remaining,
        format,
        arguments
    );

    va_end(arguments);

    if (written < 0) {
        return;
    }

    if ((size_t)written >= remaining) {
        *offset = capacity - 1U;
        buffer[*offset] = '\0';
        return;
    }

    *offset += (size_t)written;
}

void cameff_evaluation_options_default(
    cameff_evaluation_options_t *options
) {
    if (options == NULL) {
        return;
    }

    options->routing_threshold = 0.20;
    options->minimum_fused_confidence = 0.25;
    options->watch_threshold = 0.45;
    options->elevated_threshold = 0.70;
}

cameff_status_t cameff_evaluate_experts(
    const cameff_signal_frame_t *frame,
    const cameff_config_t *config,
    const cameff_region_profile_t *region,
    const cameff_evaluation_options_t *options,
    cameff_evaluation_result_t *result
) {
    cameff_expert_registry_t registry;
    cameff_expert_result_t expert_results[CAMEFF_MAX_EXPERTS];
    size_t expert_result_count;
    cameff_status_t status;

    if (frame == NULL ||
        config == NULL ||
        region == NULL ||
        options == NULL ||
        result == NULL ||
        !valid_options(options)) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }

    (void)memset(result, 0, sizeof(*result));
    (void)memset(expert_results, 0, sizeof(expert_results));

    status = cameff_classify_patterns(
        frame,
        region,
        &result->patterns
    );

    if (status != CAMEFF_STATUS_OK) {
        return status;
    }

    cameff_expert_registry_init(&registry);

    status = cameff_expert_registry_register(
        &registry,
        cameff_baseline_expert()
    );

    if (status != CAMEFF_STATUS_OK) {
        return status;
    }

    status = cameff_expert_registry_register(
        &registry,
        cameff_subduction_expert()
    );

    if (status != CAMEFF_STATUS_OK) {
        return status;
    }

    status = cameff_expert_registry_register(
        &registry,
        cameff_crustal_fault_expert()
    );

    if (status != CAMEFF_STATUS_OK) {
        return status;
    }

    status = cameff_expert_registry_register(
        &registry,
        cameff_cascade_expert()
    );

    if (status != CAMEFF_STATUS_OK) {
        return status;
    }

    status = cameff_expert_registry_evaluate(
        &registry,
        frame,
        config,
        region,
        &result->patterns,
        options->routing_threshold,
        expert_results,
        CAMEFF_MAX_EXPERTS,
        &expert_result_count
    );

    if (status != CAMEFF_STATUS_OK) {
        return status;
    }

    return cameff_fuse_expert_results(
        expert_results,
        expert_result_count,
        &result->patterns,
        options->minimum_fused_confidence,
        options->watch_threshold,
        options->elevated_threshold,
        &result->fusion
    );
}

cameff_status_t cameff_format_evaluation(
    const cameff_evaluation_result_t *result,
    char *buffer,
    size_t buffer_capacity
) {
    size_t offset;
    size_t i;

    if (result == NULL ||
        buffer == NULL ||
        buffer_capacity == 0U) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }

    buffer[0] = '\0';
    offset = 0U;

    append_text(
        buffer,
        buffer_capacity,
        &offset,
        "dominant_pattern=%s\n",
        cameff_pattern_name(
            result->patterns.dominant_pattern
        )
    );

    append_text(
        buffer,
        buffer_capacity,
        &offset,
        "classification_confidence=%.6f\n",
        result->patterns.classification_confidence
    );

    append_text(
        buffer,
        buffer_capacity,
        &offset,
        "fused_hazard_evidence=%.6f\n",
        result->fusion.hazard_evidence
    );

    append_text(
        buffer,
        buffer_capacity,
        &offset,
        "fused_preparation_evidence=%.6f\n",
        result->fusion.preparation_evidence
    );

    append_text(
        buffer,
        buffer_capacity,
        &offset,
        "fused_cascade_evidence=%.6f\n",
        result->fusion.cascade_evidence
    );

    append_text(
        buffer,
        buffer_capacity,
        &offset,
        "fused_confidence=%.6f\n",
        result->fusion.fused_confidence
    );

    append_text(
        buffer,
        buffer_capacity,
        &offset,
        "expert_coverage=%.6f\n",
        result->fusion.expert_coverage
    );

    append_text(
        buffer,
        buffer_capacity,
        &offset,
        "decision=%s\n",
        decision_name(result->fusion.decision)
    );

    for (i = 0U;
         i < result->fusion.expert_result_count;
         ++i) {
        const cameff_expert_result_t *expert;

        expert = &result->fusion.expert_results[i];

        append_text(
            buffer,
            buffer_capacity,
            &offset,
            "\nexpert=%s\n",
            expert->expert_name
        );

        append_text(
            buffer,
            buffer_capacity,
            &offset,
            "status=%s\n",
            expert_status_name(expert->status)
        );

        append_text(
            buffer,
            buffer_capacity,
            &offset,
            "applicability=%.6f\n",
            expert->applicability
        );

        append_text(
            buffer,
            buffer_capacity,
            &offset,
            "routing_strength=%.6f\n",
            expert->routing_strength
        );

        append_text(
            buffer,
            buffer_capacity,
            &offset,
            "confidence=%.6f\n",
            expert->confidence
        );

        append_text(
            buffer,
            buffer_capacity,
            &offset,
            "hazard_evidence=%.6f\n",
            expert->hazard_evidence
        );

        append_text(
            buffer,
            buffer_capacity,
            &offset,
            "preparation_evidence=%.6f\n",
            expert->preparation_evidence
        );

        append_text(
            buffer,
            buffer_capacity,
            &offset,
            "cascade_evidence=%.6f\n",
            expert->cascade_evidence
        );

        append_text(
            buffer,
            buffer_capacity,
            &offset,
            "expert_decision=%s\n",
            decision_name(expert->decision)
        );

        append_text(
            buffer,
            buffer_capacity,
            &offset,
            "explanation=%s\n",
            expert->explanation
        );
    }

    return CAMEFF_STATUS_OK;
}