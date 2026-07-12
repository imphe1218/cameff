#include "cameff/cameff.h"

#include <math.h>
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
    double value,
    double confidence
) {
    size_t i;

    (void)memset(frame, 0, sizeof(*frame));

    for (i = 0U; i < CAMEFF_SIGNAL_COUNT; ++i) {
        frame->signals[i].value = value;
        frame->signals[i].confidence = confidence;
        frame->signals[i].supporting_events = 40U;
        frame->signals[i].available = 1;
    }

    frame->selected_event_count = 40U;
    frame->data_confidence = confidence;
}

static void initialize_subduction_region(
    cameff_region_profile_t *region
) {
    (void)memset(region, 0, sizeof(*region));

    (void)snprintf(
        region->region_id,
        sizeof(region->region_id),
        "%s",
        "test-subduction-region"
    );

    region->tectonic_setting =
        CAMEFF_TECTONIC_SUBDUCTION;

    region->subduction_affinity = 1.0;
    region->crustal_fault_affinity = 0.10;
    region->transform_affinity = 0.05;
    region->volcanic_affinity = 0.05;

    region->fault_map_confidence = 0.90;
    region->catalog_completeness = 0.90;
    region->station_coverage = 0.90;
    region->regional_prior_confidence = 0.90;
}

static int test_end_to_end_subduction_evaluation(void) {
    cameff_signal_frame_t frame;
    cameff_config_t config;
    cameff_region_profile_t region;
    cameff_evaluation_options_t options;
    cameff_evaluation_result_t result;

    initialize_frame(&frame, 0.80, 0.90);
    initialize_subduction_region(&region);
    cameff_config_default(&config);
    cameff_evaluation_options_default(&options);

    CHECK(
        cameff_evaluate_experts(
            &frame,
            &config,
            &region,
            &options,
            &result
        ) == CAMEFF_STATUS_OK
    );

    CHECK(
        result.patterns.count ==
        CAMEFF_PATTERN_COUNT
    );

    CHECK(
        result.patterns.dominant_pattern ==
        CAMEFF_PATTERN_SUBDUCTION_PREPARATION
    );

    CHECK(
        result.fusion.expert_result_count == 4U
    );

    CHECK(
        result.fusion.hazard_evidence >= 0.0
    );

    CHECK(
        result.fusion.hazard_evidence <= 1.0
    );

    CHECK(
        result.fusion.preparation_evidence >= 0.0
    );

    CHECK(
        result.fusion.preparation_evidence <= 1.0
    );

    CHECK(
        result.fusion.cascade_evidence >= 0.0
    );

    CHECK(
        result.fusion.cascade_evidence <= 1.0
    );

    CHECK(
        result.fusion.fused_confidence > 0.0
    );

    CHECK(
        strcmp(
            result.fusion.expert_results[0].expert_name,
            "baseline"
        ) == 0
    );

    CHECK(
        strcmp(
            result.fusion.expert_results[1].expert_name,
            "subduction"
        ) == 0
    );

    CHECK(
        strcmp(
            result.fusion.expert_results[2].expert_name,
            "crustal-fault"
        ) == 0
    );

    CHECK(
        strcmp(
            result.fusion.expert_results[3].expert_name,
            "seismic-cascade"
        ) == 0
    );

    CHECK(
        result.fusion.expert_results[0].status ==
        CAMEFF_EXPERT_OK
    );

    return 0;
}

static int test_quiet_background_evaluation(void) {
    cameff_signal_frame_t frame;
    cameff_config_t config;
    cameff_region_profile_t region;
    cameff_evaluation_options_t options;
    cameff_evaluation_result_t result;

    initialize_frame(&frame, 0.0, 1.0);
    initialize_subduction_region(&region);
    cameff_config_default(&config);
    cameff_evaluation_options_default(&options);

    CHECK(
        cameff_evaluate_experts(
            &frame,
            &config,
            &region,
            &options,
            &result
        ) == CAMEFF_STATUS_OK
    );

    CHECK(
        result.patterns.dominant_pattern ==
        CAMEFF_PATTERN_BACKGROUND
    );

    CHECK(
        result.fusion.decision ==
        CAMEFF_DECISION_BACKGROUND
    );

    return 0;
}

static int test_explanation_output(void) {
    cameff_signal_frame_t frame;
    cameff_config_t config;
    cameff_region_profile_t region;
    cameff_evaluation_options_t options;
    cameff_evaluation_result_t result;
    char explanation[8192];

    initialize_frame(&frame, 0.75, 0.90);
    initialize_subduction_region(&region);
    cameff_config_default(&config);
    cameff_evaluation_options_default(&options);

    CHECK(
        cameff_evaluate_experts(
            &frame,
            &config,
            &region,
            &options,
            &result
        ) == CAMEFF_STATUS_OK
    );

    CHECK(
        cameff_format_evaluation(
            &result,
            explanation,
            sizeof(explanation)
        ) == CAMEFF_STATUS_OK
    );

    CHECK(
        strstr(
            explanation,
            "dominant_pattern="
        ) != NULL
    );

    CHECK(
        strstr(
            explanation,
            "fused_hazard_evidence="
        ) != NULL
    );

    CHECK(
        strstr(
            explanation,
            "expert=baseline"
        ) != NULL
    );

    CHECK(
        strstr(
            explanation,
            "expert=subduction"
        ) != NULL
    );

    CHECK(
        strstr(
            explanation,
            "routing_strength="
        ) != NULL
    );

    CHECK(
        strstr(
            explanation,
            "decision="
        ) != NULL
    );

    return 0;
}

static int test_invalid_options(void) {
    cameff_signal_frame_t frame;
    cameff_config_t config;
    cameff_region_profile_t region;
    cameff_evaluation_options_t options;
    cameff_evaluation_result_t result;

    initialize_frame(&frame, 0.50, 0.80);
    initialize_subduction_region(&region);
    cameff_config_default(&config);
    cameff_evaluation_options_default(&options);

    options.watch_threshold = 0.80;
    options.elevated_threshold = 0.70;

    CHECK(
        cameff_evaluate_experts(
            &frame,
            &config,
            &region,
            &options,
            &result
        ) == CAMEFF_STATUS_INVALID_ARGUMENT
    );

    return 0;
}

int main(void) {
    CHECK(
        test_end_to_end_subduction_evaluation() == 0
    );

    CHECK(
        test_quiet_background_evaluation() == 0
    );

    CHECK(
        test_explanation_output() == 0
    );

    CHECK(
        test_invalid_options() == 0
    );

    puts("end-to-end evaluation tests passed");
    return 0;
}