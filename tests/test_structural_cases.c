#include "cameff/cameff.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf( \
            stderr, \
            "check failed: %s at line %d\n", \
            #condition, \
            __LINE__ \
        ); \
        return 1; \
    } \
} while (0)

static void initialize_frame(
    cameff_signal_frame_t *frame,
    double activity,
    double acceleration,
    double concentration,
    double magnitude,
    double depth,
    double b_value,
    double completeness,
    double fault_map,
    double confidence
) {
    size_t i;

    (void)memset(frame, 0, sizeof(*frame));

    for (i = 0U; i < CAMEFF_SIGNAL_COUNT; ++i) {
        frame->signals[i].confidence = confidence;
        frame->signals[i].supporting_events = 50U;
        frame->signals[i].available = 1;
    }

    frame->signals[
        CAMEFF_SIGNAL_ACTIVITY_RATE
    ].value = activity;

    frame->signals[
        CAMEFF_SIGNAL_TEMPORAL_ACCELERATION
    ].value = acceleration;

    frame->signals[
        CAMEFF_SIGNAL_SPATIAL_CONCENTRATION
    ].value = concentration;

    frame->signals[
        CAMEFF_SIGNAL_MAGNITUDE_TREND
    ].value = magnitude;

    frame->signals[
        CAMEFF_SIGNAL_DEPTH_MIGRATION
    ].value = depth;

    frame->signals[
        CAMEFF_SIGNAL_B_VALUE_ANOMALY
    ].value = b_value;

    frame->signals[
        CAMEFF_SIGNAL_CATALOG_COMPLETENESS
    ].value = completeness;

    frame->signals[
        CAMEFF_SIGNAL_FAULT_MAP_CONFIDENCE
    ].value = fault_map;

    frame->selected_event_count = 50U;
    frame->data_confidence = confidence;
}

static void initialize_region(
    cameff_region_profile_t *region,
    const char *region_id
) {
    (void)memset(region, 0, sizeof(*region));

    (void)snprintf(
        region->region_id,
        sizeof(region->region_id),
        "%s",
        region_id
    );

    region->fault_map_confidence = 0.80;
    region->catalog_completeness = 0.90;
    region->station_coverage = 0.90;
    region->regional_prior_confidence = 0.90;
}

static int find_expert(
    const cameff_evaluation_result_t *result,
    const char *expert_name
) {
    size_t i;

    for (
        i = 0U;
        i < result->fusion.expert_result_count;
        ++i
    ) {
        if (strcmp(
                result->fusion
                    .expert_results[i]
                    .expert_name,
                expert_name
            ) == 0) {
            return (int)i;
        }
    }

    return -1;
}

static int evaluate_case(
    const cameff_signal_frame_t *frame,
    const cameff_region_profile_t *region,
    cameff_evaluation_result_t *result
) {
    cameff_config_t config;
    cameff_evaluation_options_t options;

    cameff_config_default(&config);
    cameff_evaluation_options_default(&options);

    return cameff_evaluate_experts(
        frame,
        &config,
        region,
        &options,
        result
    ) == CAMEFF_STATUS_OK;
}

/*
 * Structural analogue of the 2011 Japan case:
 * strong subduction affinity with elevated temporal,
 * magnitude and depth-related channels.
 */
static int test_japan_structure(void) {
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_evaluation_result_t result;
    int subduction_index;

    initialize_frame(
        &frame,
        0.78,
        0.88,
        0.76,
        0.84,
        0.80,
        0.72,
        0.92,
        0.90,
        0.92
    );

    initialize_region(&region, "japan-structural");

    region.tectonic_setting =
        CAMEFF_TECTONIC_SUBDUCTION;

    region.subduction_affinity = 1.00;
    region.crustal_fault_affinity = 0.15;
    region.transform_affinity = 0.05;
    region.volcanic_affinity = 0.10;

    CHECK(evaluate_case(&frame, &region, &result));

    CHECK(
        result.patterns.dominant_pattern ==
        CAMEFF_PATTERN_SUBDUCTION_PREPARATION
    );

    subduction_index =
        find_expert(&result, "subduction");

    CHECK(subduction_index >= 0);

    CHECK(
        result.fusion.expert_results[
            (size_t)subduction_index
        ].status == CAMEFF_EXPERT_OK
    );

    CHECK(
        result.fusion.expert_results[
            (size_t)subduction_index
        ].preparation_evidence > 0.50
    );

    return 0;
}

/*
 * Structural analogue of the Russia case:
 * subduction setting with strong activity and concentration.
 */
static int test_russia_structure(void) {
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_evaluation_result_t result;

    initialize_frame(
        &frame,
        0.82,
        0.78,
        0.83,
        0.76,
        0.74,
        0.70,
        0.88,
        0.86,
        0.88
    );

    initialize_region(&region, "russia-structural");

    region.tectonic_setting =
        CAMEFF_TECTONIC_SUBDUCTION;

    region.subduction_affinity = 0.95;
    region.crustal_fault_affinity = 0.20;
    region.transform_affinity = 0.05;

    CHECK(evaluate_case(&frame, &region, &result));

    CHECK(
        result.patterns.scores[
            CAMEFF_PATTERN_SUBDUCTION_PREPARATION
        ].membership >
        result.patterns.scores[
            CAMEFF_PATTERN_CRUSTAL_FAULT_ACTIVATION
        ].membership
    );

    CHECK(
        result.fusion.preparation_evidence > 0.0
    );

    return 0;
}

/*
 * Structural analogue of the Cebu case:
 * concentrated crustal activity with uncertain fault mapping.
 */
static int test_cebu_structure(void) {
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_evaluation_result_t result;
    int crustal_index;

    initialize_frame(
        &frame,
        0.76,
        0.72,
        0.94,
        0.70,
        0.38,
        0.68,
        0.82,
        0.25,
        0.84
    );

    initialize_region(&region, "cebu-structural");

    region.tectonic_setting =
        CAMEFF_TECTONIC_CRUSTAL;

    region.subduction_affinity = 0.15;
    region.crustal_fault_affinity = 0.92;
    region.transform_affinity = 0.10;

    region.fault_map_confidence = 0.20;
    region.catalog_completeness = 0.82;
    region.station_coverage = 0.78;
    region.regional_prior_confidence = 0.75;

    CHECK(evaluate_case(&frame, &region, &result));

    CHECK(
        result.patterns.dominant_pattern ==
        CAMEFF_PATTERN_CRUSTAL_FAULT_ACTIVATION
    );

    crustal_index =
        find_expert(&result, "crustal-fault");

    CHECK(crustal_index >= 0);

    CHECK(
        result.fusion.expert_results[
            (size_t)crustal_index
        ].status == CAMEFF_EXPERT_OK
    );

    CHECK(
        result.fusion.expert_results[
            (size_t)crustal_index
        ].preparation_evidence > 0.45
    );

    return 0;
}

/*
 * Structural analogue of the Davao case:
 * a subduction region with meaningful crustal ambiguity.
 */
static int test_davao_structure(void) {
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_evaluation_result_t result;

    initialize_frame(
        &frame,
        0.74,
        0.80,
        0.78,
        0.74,
        0.82,
        0.66,
        0.84,
        0.72,
        0.85
    );

    initialize_region(&region, "davao-structural");

    region.tectonic_setting =
        CAMEFF_TECTONIC_MIXED;

    region.subduction_affinity = 0.88;
    region.crustal_fault_affinity = 0.48;
    region.transform_affinity = 0.10;

    region.fault_map_confidence = 0.65;
    region.catalog_completeness = 0.84;
    region.station_coverage = 0.82;
    region.regional_prior_confidence = 0.82;

    CHECK(evaluate_case(&frame, &region, &result));

    CHECK(
        result.patterns.scores[
            CAMEFF_PATTERN_SUBDUCTION_PREPARATION
        ].membership >
        result.patterns.scores[
            CAMEFF_PATTERN_CRUSTAL_FAULT_ACTIVATION
        ].membership
    );

    CHECK(
        result.fusion.expert_result_count == 4U
    );

    CHECK(
        result.fusion.expert_coverage > 0.0
    );

    return 0;
}

/*
 * Structural analogue of the Venezuela case:
 * transform-dominant regional context.
 *
 * Milestone 2 does not yet implement a transform expert.
 * This test intentionally documents that architectural gap.
 */
static int test_venezuela_structure(void) {
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_evaluation_result_t result;
    int subduction_index;
    int crustal_index;

    initialize_frame(
        &frame,
        0.68,
        0.72,
        0.80,
        0.66,
        0.42,
        0.60,
        0.86,
        0.75,
        0.88
    );

    initialize_region(
        &region,
        "venezuela-structural"
    );

    region.tectonic_setting =
        CAMEFF_TECTONIC_TRANSFORM;

    region.subduction_affinity = 0.05;
    region.crustal_fault_affinity = 0.20;
    region.transform_affinity = 1.00;
    region.volcanic_affinity = 0.00;

    region.fault_map_confidence = 0.75;
    region.catalog_completeness = 0.86;
    region.station_coverage = 0.82;
    region.regional_prior_confidence = 0.90;

    CHECK(evaluate_case(&frame, &region, &result));

    CHECK(
        result.patterns.dominant_pattern ==
        CAMEFF_PATTERN_TRANSFORM_FAULT_ACTIVITY
    );

    subduction_index =
        find_expert(&result, "subduction");

    crustal_index =
        find_expert(&result, "crustal-fault");

    CHECK(subduction_index >= 0);
    CHECK(crustal_index >= 0);

    CHECK(
        result.fusion.expert_results[
            (size_t)subduction_index
        ].status == CAMEFF_EXPERT_ABSTAIN
    );

    /*
     * The current framework has no transform-specific expert.
     * The baseline and cascade experts may still contribute,
     * but transform interpretation remains incomplete.
     */
    CHECK(
        result.patterns.scores[
            CAMEFF_PATTERN_TRANSFORM_FAULT_ACTIVITY
        ].membership >
        result.patterns.scores[
            CAMEFF_PATTERN_SUBDUCTION_PREPARATION
        ].membership
    );

    return 0;
}

int main(void) {
    CHECK(test_japan_structure() == 0);
    CHECK(test_russia_structure() == 0);
    CHECK(test_cebu_structure() == 0);
    CHECK(test_davao_structure() == 0);
    CHECK(test_venezuela_structure() == 0);

    puts("structural hindcast case tests passed");
    return 0;
}