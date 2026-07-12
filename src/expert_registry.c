#include "cameff/expert_registry.h"

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

static double pattern_membership(
    const cameff_pattern_result_t *patterns,
    cameff_pattern_id_t pattern_id
) {
    size_t i;

    /*
     * UNKNOWN is used by process-neutral experts such as the
     * Milestone 1 baseline expert. Such experts are globally routed.
     */
    if (pattern_id == CAMEFF_PATTERN_UNKNOWN) {
        return 1.0;
    }

    if (patterns == NULL) {
        return 0.0;
    }

    for (i = 0U; i < patterns->count; ++i) {
        if (patterns->scores[i].pattern_id == pattern_id) {
            return clamp01(patterns->scores[i].membership);
        }
    }

    return 0.0;
}

static void initialize_abstention(
    const cameff_expert_t *expert,
    double applicability,
    double routing_strength,
    cameff_expert_result_t *result
) {
    (void)memset(result, 0, sizeof(*result));

    (void)snprintf(
        result->expert_name,
        sizeof(result->expert_name),
        "%s",
        expert->name
    );

    result->applicability = applicability;
    result->routing_strength = routing_strength;
    result->decision = CAMEFF_DECISION_INSUFFICIENT_DATA;
    result->status = CAMEFF_EXPERT_ABSTAIN;

    (void)snprintf(
        result->explanation,
        sizeof(result->explanation),
        "Expert routing strength %.6f is below the activation threshold.",
        routing_strength
    );
}

void cameff_expert_registry_init(
    cameff_expert_registry_t *registry
) {
    if (registry == NULL) {
        return;
    }

    (void)memset(registry, 0, sizeof(*registry));
}

cameff_status_t cameff_expert_registry_register(
    cameff_expert_registry_t *registry,
    const cameff_expert_t *expert
) {
    size_t i;

    if (registry == NULL ||
        expert == NULL ||
        expert->name == NULL ||
        expert->applicability == NULL ||
        expert->evaluate == NULL) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }

    if (registry->count >= CAMEFF_MAX_EXPERTS) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }

    for (i = 0U; i < registry->count; ++i) {
        if (strcmp(
                registry->experts[i]->name,
                expert->name
            ) == 0) {
            return CAMEFF_STATUS_INVALID_ARGUMENT;
        }
    }

    registry->experts[registry->count] = expert;
    ++registry->count;

    return CAMEFF_STATUS_OK;
}

size_t cameff_expert_registry_count(
    const cameff_expert_registry_t *registry
) {
    if (registry == NULL) {
        return 0U;
    }

    return registry->count;
}

const cameff_expert_t *cameff_expert_registry_get(
    const cameff_expert_registry_t *registry,
    size_t index
) {
    if (registry == NULL || index >= registry->count) {
        return NULL;
    }

    return registry->experts[index];
}

double cameff_expert_routing_strength(
    const cameff_expert_t *expert,
    const cameff_signal_frame_t *frame,
    const cameff_region_profile_t *region,
    const cameff_pattern_result_t *patterns
) {
    double applicability;
    double membership;

    if (expert == NULL ||
        frame == NULL ||
        expert->applicability == NULL) {
        return 0.0;
    }

    applicability = clamp01(
        expert->applicability(
            frame,
            region,
            patterns
        )
    );

    membership = pattern_membership(
        patterns,
        expert->primary_pattern
    );

    return clamp01(applicability * membership);
}

cameff_status_t cameff_expert_registry_evaluate(
    const cameff_expert_registry_t *registry,
    const cameff_signal_frame_t *frame,
    const cameff_config_t *config,
    const cameff_region_profile_t *region,
    const cameff_pattern_result_t *patterns,
    double routing_threshold,
    cameff_expert_result_t *results,
    size_t result_capacity,
    size_t *result_count
) {
    size_t i;

    if (registry == NULL ||
        frame == NULL ||
        config == NULL ||
        results == NULL ||
        result_count == NULL ||
        !isfinite(routing_threshold) ||
        routing_threshold < 0.0 ||
        routing_threshold > 1.0 ||
        result_capacity < registry->count) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }

    *result_count = 0U;

    for (i = 0U; i < registry->count; ++i) {
        const cameff_expert_t *expert;
        cameff_expert_result_t *result;
        cameff_status_t status;
        double applicability;
        double routing_strength;

        expert = registry->experts[i];
        result = &results[i];

        applicability = clamp01(
            expert->applicability(
                frame,
                region,
                patterns
            )
        );

        routing_strength =
            cameff_expert_routing_strength(
                expert,
                frame,
                region,
                patterns
            );

        if (routing_strength < routing_threshold) {
            initialize_abstention(
                expert,
                applicability,
                routing_strength,
                result
            );

            ++(*result_count);
            continue;
        }

        (void)memset(result, 0, sizeof(*result));

        status = expert->evaluate(
            frame,
            config,
            region,
            patterns,
            result
        );

        if (status != CAMEFF_STATUS_OK) {
            return status;
        }

        result->applicability = applicability;
        result->routing_strength = routing_strength;

        if (result->expert_name[0] == '\0') {
            (void)snprintf(
                result->expert_name,
                sizeof(result->expert_name),
                "%s",
                expert->name
            );
        }

        ++(*result_count);
    }

    return CAMEFF_STATUS_OK;
}