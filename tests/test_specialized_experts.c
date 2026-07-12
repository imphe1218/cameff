#include "cameff/cameff.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed: %s at line %d\n", \
                #condition, __LINE__); \
        return 1; \
    } \
} while (0)

static void initialize_frame(
    cameff_signal_frame_t *frame,
    double value
) {
    size_t i;

    (void)memset(frame, 0, sizeof(*frame));

    for (i = 0U; i < CAMEFF_SIGNAL_COUNT; ++i) {
        frame->signals[i].value = value;
        frame->signals[i].confidence = 0.90;
        frame->signals[i].supporting_events = 30U;
        frame->signals[i].available = 1;
    }

    frame->selected_event_count = 30U;
    frame->data_confidence = 0.90;
}

static void initialize_region(
    cameff_region_profile_t *region
) {
    (void)memset(region, 0, sizeof(*region));

    region->fault_map_confidence = 0.80;
    region->catalog_completeness = 0.90;
    region->station_coverage = 0.90;
    region->regional_prior_confidence = 0.90;
}

static int evaluate_expert(
    const cameff_expert_t *expert,
    cameff_signal_frame_t *frame,
    cameff_region_profile_t *region,
    cameff_expert_result_t *result
) {
    cameff_config_t config;

    cameff_config_default(&config);

    CHECK(expert != NULL);
    CHECK(expert->applicability != NULL);
    CHECK(expert->evaluate != NULL);

    CHECK(
        expert->evaluate(
            frame,
            &config,
            region,
            NULL,
            result
        ) == CAMEFF_STATUS_OK
    );

    CHECK(result->hazard_evidence >= 0.0);
    CHECK(result->hazard_evidence <= 1.0);
    CHECK(result->preparation_evidence >= 0.0);
    CHECK(result->preparation_evidence <= 1.0);
    CHECK(result->cascade_evidence >= 0.0);
    CHECK(result->cascade_evidence <= 1.0);
    CHECK(result->confidence >= 0.0);
    CHECK(result->confidence <= 1.0);

    return 0;
}

static int test_subduction_expert(void) {
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_expert_result_t result;
    const cameff_expert_t *expert;

    initialize_frame(&frame, 0.80);
    initialize_region(&region);

    region.tectonic_setting = CAMEFF_TECTONIC_SUBDUCTION;
    region.subduction_affinity = 1.0;

    expert = cameff_subduction_expert();

    CHECK(
        expert->applicability(
            &frame,
            &region,
            NULL
        ) > 0.80
    );

    CHECK(evaluate_expert(expert, &frame, &region, &result) == 0);
    CHECK(result.status == CAMEFF_EXPERT_OK);
    CHECK(result.preparation_evidence > 0.50);
    CHECK(result.cascade_evidence == 0.0);

    return 0;
}

static int test_crustal_fault_uncertainty_is_guarded(void) {
    cameff_signal_frame_t quiet_frame;
    cameff_signal_frame_t active_frame;
    cameff_region_profile_t region;
    cameff_expert_result_t quiet_result;
    cameff_expert_result_t active_result;
    const cameff_expert_t *expert;

    initialize_frame(&quiet_frame, 0.0);
    initialize_frame(&active_frame, 0.80);
    initialize_region(&region);

    region.tectonic_setting = CAMEFF_TECTONIC_CRUSTAL;
    region.crustal_fault_affinity = 0.70;
    region.fault_map_confidence = 0.10;

    expert = cameff_crustal_fault_expert();

    CHECK(
        evaluate_expert(
            expert,
            &quiet_frame,
            &region,
            &quiet_result
        ) == 0
    );

    CHECK(
        evaluate_expert(
            expert,
            &active_frame,
            &region,
            &active_result
        ) == 0
    );

    CHECK(
        active_result.preparation_evidence >
        quiet_result.preparation_evidence
    );

    CHECK(
        quiet_result.hazard_evidence < 0.45
    );

    return 0;
}

static int test_cascade_expert(void) {
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_expert_result_t result;
    const cameff_expert_t *expert;

    initialize_frame(&frame, 0.85);
    initialize_region(&region);

    expert = cameff_cascade_expert();

    CHECK(
        expert->applicability(
            &frame,
            &region,
            NULL
        ) > 0.70
    );

    CHECK(evaluate_expert(expert, &frame, &region, &result) == 0);
    CHECK(result.status == CAMEFF_EXPERT_OK);
    CHECK(result.cascade_evidence > 0.60);
    CHECK(
        result.cascade_evidence >
        result.preparation_evidence
    );

    return 0;
}

int main(void) {
    CHECK(test_subduction_expert() == 0);
    CHECK(test_crustal_fault_uncertainty_is_guarded() == 0);
    CHECK(test_cascade_expert() == 0);

    puts("specialized expert tests passed");
    return 0;
}