#include "cameff/cameff.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define TEST_TOLERANCE 1e-12

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed: %s at line %d\n", \
                #condition, __LINE__); \
        return 1; \
    } \
} while (0)

#define CHECK_CLOSE(actual, expected, tolerance) do { \
    if (fabs((actual) - (expected)) > (tolerance)) { \
        fprintf(stderr, \
                "check failed: %s ~= %s at line %d\n", \
                #actual, #expected, __LINE__); \
        return 1; \
    } \
} while (0)

static void initialize_frame(
    cameff_signal_frame_t *frame
) {
    size_t i;

    (void)memset(frame, 0, sizeof(*frame));

    for (i = 0U; i < CAMEFF_SIGNAL_COUNT; ++i) {
        frame->signals[i].value = 0.70;
        frame->signals[i].confidence = 0.90;
        frame->signals[i].supporting_events = 25U;
        frame->signals[i].available = 1;
    }

    frame->selected_event_count = 25U;
    frame->data_confidence = 0.90;
}

static void initialize_region(
    cameff_region_profile_t *region
) {
    (void)memset(region, 0, sizeof(*region));

    region->tectonic_setting = CAMEFF_TECTONIC_SUBDUCTION;
    region->subduction_affinity = 1.0;
    region->fault_map_confidence = 0.90;
    region->catalog_completeness = 0.90;
    region->station_coverage = 0.90;
    region->regional_prior_confidence = 0.90;
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

    return region->subduction_affinity;
}

static cameff_status_t subduction_evaluate(
    const cameff_signal_frame_t *frame,
    const cameff_config_t *config,
    const cameff_region_profile_t *region,
    const cameff_pattern_result_t *patterns,
    cameff_expert_result_t *result
) {
    (void)frame;
    (void)config;
    (void)region;
    (void)patterns;

    if (result == NULL) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }

    (void)memset(result, 0, sizeof(*result));

    (void)snprintf(
        result->expert_name,
        sizeof(result->expert_name),
        "%s",
        "test-subduction"
    );

    result->hazard_evidence = 0.75;
    result->preparation_evidence = 0.80;
    result->confidence = 0.85;
    result->decision = CAMEFF_DECISION_ELEVATED;
    result->status = CAMEFF_EXPERT_OK;

    return CAMEFF_STATUS_OK;
}

static const cameff_expert_t TEST_SUBDUCTION_EXPERT = {
    "test-subduction",
    CAMEFF_PATTERN_SUBDUCTION_PREPARATION,
    subduction_applicability,
    subduction_evaluate
};

static int test_registration(void) {
    cameff_expert_registry_t registry;

    cameff_expert_registry_init(&registry);

    CHECK(cameff_expert_registry_count(&registry) == 0U);

    CHECK(
        cameff_expert_registry_register(
            &registry,
            cameff_baseline_expert()
        ) == CAMEFF_STATUS_OK
    );

    CHECK(
        cameff_expert_registry_register(
            &registry,
            &TEST_SUBDUCTION_EXPERT
        ) == CAMEFF_STATUS_OK
    );

    CHECK(cameff_expert_registry_count(&registry) == 2U);

    CHECK(
        cameff_expert_registry_get(&registry, 0U) ==
        cameff_baseline_expert()
    );

    CHECK(
        cameff_expert_registry_get(&registry, 2U) ==
        NULL
    );

    CHECK(
        cameff_expert_registry_register(
            &registry,
            &TEST_SUBDUCTION_EXPERT
        ) == CAMEFF_STATUS_INVALID_ARGUMENT
    );

    return 0;
}

static int test_routing_strength(void) {
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_pattern_result_t patterns;
    double routing_strength;

    initialize_frame(&frame);
    initialize_region(&region);
    (void)memset(&patterns, 0, sizeof(patterns));

    patterns.count = CAMEFF_PATTERN_COUNT;

    patterns.scores[
        CAMEFF_PATTERN_SUBDUCTION_PREPARATION
    ].pattern_id =
        CAMEFF_PATTERN_SUBDUCTION_PREPARATION;

    patterns.scores[
        CAMEFF_PATTERN_SUBDUCTION_PREPARATION
    ].membership = 0.80;

    routing_strength =
        cameff_expert_routing_strength(
            &TEST_SUBDUCTION_EXPERT,
            &frame,
            &region,
            &patterns
        );

    CHECK_CLOSE(routing_strength, 0.80, TEST_TOLERANCE);

    routing_strength =
        cameff_expert_routing_strength(
            cameff_baseline_expert(),
            &frame,
            &region,
            &patterns
        );

    CHECK_CLOSE(routing_strength, 1.0, TEST_TOLERANCE);

    return 0;
}

static int test_activation_and_abstention(void) {
    cameff_expert_registry_t registry;
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_pattern_result_t patterns;
    cameff_config_t config;
    cameff_expert_result_t results[CAMEFF_MAX_EXPERTS];
    size_t result_count;

    initialize_frame(&frame);
    initialize_region(&region);
    cameff_config_default(&config);

    (void)memset(&patterns, 0, sizeof(patterns));
    patterns.count = CAMEFF_PATTERN_COUNT;

    patterns.scores[
        CAMEFF_PATTERN_SUBDUCTION_PREPARATION
    ].pattern_id =
        CAMEFF_PATTERN_SUBDUCTION_PREPARATION;

    patterns.scores[
        CAMEFF_PATTERN_SUBDUCTION_PREPARATION
    ].membership = 0.10;

    cameff_expert_registry_init(&registry);

    CHECK(
        cameff_expert_registry_register(
            &registry,
            cameff_baseline_expert()
        ) == CAMEFF_STATUS_OK
    );

    CHECK(
        cameff_expert_registry_register(
            &registry,
            &TEST_SUBDUCTION_EXPERT
        ) == CAMEFF_STATUS_OK
    );

    CHECK(
        cameff_expert_registry_evaluate(
            &registry,
            &frame,
            &config,
            &region,
            &patterns,
            0.20,
            results,
            CAMEFF_MAX_EXPERTS,
            &result_count
        ) == CAMEFF_STATUS_OK
    );

    CHECK(result_count == 2U);

    CHECK(results[0].status == CAMEFF_EXPERT_OK);
    CHECK_CLOSE(
        results[0].routing_strength,
        1.0,
        TEST_TOLERANCE
    );

    CHECK(results[1].status == CAMEFF_EXPERT_ABSTAIN);
    CHECK_CLOSE(
        results[1].routing_strength,
        0.10,
        TEST_TOLERANCE
    );

    CHECK(
        results[1].decision ==
        CAMEFF_DECISION_INSUFFICIENT_DATA
    );

    return 0;
}

int main(void) {
    CHECK(test_registration() == 0);
    CHECK(test_routing_strength() == 0);
    CHECK(test_activation_and_abstention() == 0);

    puts("expert registry tests passed");
    return 0;
}