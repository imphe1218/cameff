#include "cameff/fusion.h"

#include <math.h>
#include <string.h>

#define CAMEFF_FUSION_EPSILON 1e-12

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

static int valid_thresholds(
    double minimum_confidence,
    double watch_threshold,
    double elevated_threshold
) {
    if (!isfinite(minimum_confidence) ||
        !isfinite(watch_threshold) ||
        !isfinite(elevated_threshold)) {
        return 0;
    }

    if (minimum_confidence < 0.0 ||
        minimum_confidence > 1.0) {
        return 0;
    }

    if (watch_threshold < 0.0 ||
        watch_threshold > 1.0) {
        return 0;
    }

    if (elevated_threshold < 0.0 ||
        elevated_threshold > 1.0) {
        return 0;
    }

    if (watch_threshold > elevated_threshold) {
        return 0;
    }

    return 1;
}

static cameff_decision_level_t fused_decision(
    double hazard_evidence,
    double fused_confidence,
    double minimum_confidence,
    double watch_threshold,
    double elevated_threshold,
    size_t active_expert_count
) {
    if (active_expert_count == 0U ||
        fused_confidence < minimum_confidence) {
        return CAMEFF_DECISION_INSUFFICIENT_DATA;
    }

    if (hazard_evidence >= elevated_threshold) {
        return CAMEFF_DECISION_ELEVATED;
    }

    if (hazard_evidence >= watch_threshold) {
        return CAMEFF_DECISION_WATCH;
    }

    return CAMEFF_DECISION_BACKGROUND;
}

cameff_status_t cameff_fuse_expert_results(
    const cameff_expert_result_t *expert_results,
    size_t expert_result_count,
    const cameff_pattern_result_t *patterns,
    double minimum_confidence,
    double watch_threshold,
    double elevated_threshold,
    cameff_multi_expert_result_t *result
) {
    double weighted_hazard;
    double weighted_preparation;
    double weighted_cascade;
    double total_effective_weight;
    double total_active_routing;
    double confidence_numerator;
    double classification_confidence;
    size_t active_expert_count;
    size_t i;

    if (expert_results == NULL ||
        result == NULL ||
        expert_result_count > CAMEFF_MAX_EXPERTS ||
        !valid_thresholds(
            minimum_confidence,
            watch_threshold,
            elevated_threshold
        )) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }

    (void)memset(result, 0, sizeof(*result));

    result->expert_result_count = expert_result_count;
    result->dominant_pattern = CAMEFF_PATTERN_UNKNOWN;

    if (patterns != NULL) {
        result->dominant_pattern =
            patterns->dominant_pattern;
    }

    weighted_hazard = 0.0;
    weighted_preparation = 0.0;
    weighted_cascade = 0.0;
    total_effective_weight = 0.0;
    total_active_routing = 0.0;
    confidence_numerator = 0.0;
    active_expert_count = 0U;

    for (i = 0U; i < expert_result_count; ++i) {
        double routing_strength;
        double confidence;
        double effective_weight;

        result->expert_results[i] = expert_results[i];

        if (expert_results[i].status != CAMEFF_EXPERT_OK) {
            continue;
        }

        routing_strength =
            clamp01(expert_results[i].routing_strength);

        confidence =
            clamp01(expert_results[i].confidence);

        effective_weight =
            routing_strength * confidence;

        if (effective_weight <= CAMEFF_FUSION_EPSILON) {
            continue;
        }

        weighted_hazard +=
            effective_weight *
            clamp01(expert_results[i].hazard_evidence);

        weighted_preparation +=
            effective_weight *
            clamp01(expert_results[i].preparation_evidence);

        weighted_cascade +=
            effective_weight *
            clamp01(expert_results[i].cascade_evidence);

        confidence_numerator +=
            routing_strength * confidence;

        total_effective_weight += effective_weight;
        total_active_routing += routing_strength;
        ++active_expert_count;
    }

    if (expert_result_count > 0U) {
        result->expert_coverage =
            clamp01(
                total_active_routing /
                (double)expert_result_count
            );
    }

    if (total_effective_weight >
        CAMEFF_FUSION_EPSILON) {
        result->hazard_evidence =
            clamp01(
                weighted_hazard /
                total_effective_weight
            );

        result->preparation_evidence =
            clamp01(
                weighted_preparation /
                total_effective_weight
            );

        result->cascade_evidence =
            clamp01(
                weighted_cascade /
                total_effective_weight
            );
    }

    classification_confidence = 1.0;

    if (patterns != NULL) {
        classification_confidence =
            clamp01(
                patterns->classification_confidence
            );
    }

    if (total_active_routing >
        CAMEFF_FUSION_EPSILON) {
        result->fused_confidence =
            clamp01(
                (
                    confidence_numerator /
                    total_active_routing
                ) *
                classification_confidence
            );
    }

    result->decision =
        fused_decision(
            result->hazard_evidence,
            result->fused_confidence,
            minimum_confidence,
            watch_threshold,
            elevated_threshold,
            active_expert_count
        );

    return CAMEFF_STATUS_OK;
}