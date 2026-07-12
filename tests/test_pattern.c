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
    cameff_signal_frame_t *frame,
    double value,
    double confidence
) {
    size_t i;

    (void)memset(frame, 0, sizeof(*frame));

    for (i = 0U; i < CAMEFF_SIGNAL_COUNT; ++i) {
        frame->signals[i].value = value;
        frame->signals[i].confidence = confidence;
        frame->signals[i].supporting_events = 30U;
        frame->signals[i].available = 1;
    }

    frame->selected_event_count = 30U;
    frame->data_confidence = confidence;
}

static void initialize_region(
    cameff_region_profile_t *region
) {
    (void)memset(region, 0, sizeof(*region));

    region->tectonic_setting = CAMEFF_TECTONIC_UNKNOWN;
    region->fault_map_confidence = 0.80;
    region->catalog_completeness = 0.90;
    region->station_coverage = 0.90;
    region->regional_prior_confidence = 0.90;
}

static int test_memberships_are_normalized(void) {
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_pattern_result_t result;
    cameff_status_t status;
    double total;
    size_t i;

    initialize_frame(&frame, 0.50, 0.80);
    initialize_region(&region);

    region.subduction_affinity = 1.0;
    region.tectonic_setting = CAMEFF_TECTONIC_SUBDUCTION;

    status = cameff_classify_patterns(
        &frame,
        &region,
        &result
    );

    CHECK(status == CAMEFF_STATUS_OK);
    CHECK(result.count == CAMEFF_PATTERN_COUNT);

    total = 0.0;

    for (i = 0U; i < result.count; ++i) {
        CHECK(result.scores[i].membership >= 0.0);
        CHECK(result.scores[i].membership <= 1.0);
        CHECK(result.scores[i].confidence >= 0.0);
        CHECK(result.scores[i].confidence <= 1.0);

        total += result.scores[i].membership;
    }

    CHECK_CLOSE(total, 1.0, TEST_TOLERANCE);

    return 0;
}

static int test_quiet_signals_favor_background(void) {
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_pattern_result_t result;

    initialize_frame(&frame, 0.0, 1.0);
    initialize_region(&region);

    CHECK(
        cameff_classify_patterns(
            &frame,
            &region,
            &result
        ) == CAMEFF_STATUS_OK
    );

    CHECK(
        result.dominant_pattern ==
        CAMEFF_PATTERN_BACKGROUND
    );

    return 0;
}

static int test_subduction_profile_favors_subduction(void) {
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_pattern_result_t result;

    initialize_frame(&frame, 0.75, 0.90);
    initialize_region(&region);

    region.tectonic_setting = CAMEFF_TECTONIC_SUBDUCTION;
    region.subduction_affinity = 1.0;

    CHECK(
        cameff_classify_patterns(
            &frame,
            &region,
            &result
        ) == CAMEFF_STATUS_OK
    );

    CHECK(
        result.scores[
            CAMEFF_PATTERN_SUBDUCTION_PREPARATION
        ].membership >
        result.scores[
            CAMEFF_PATTERN_TRANSFORM_FAULT_ACTIVITY
        ].membership
    );

    CHECK(
        result.scores[
            CAMEFF_PATTERN_SUBDUCTION_PREPARATION
        ].membership >
        result.scores[
            CAMEFF_PATTERN_VOLCANIC_UNREST
        ].membership
    );

    return 0;
}

static int test_crustal_profile_favors_crustal_pattern(void) {
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_pattern_result_t result;

    initialize_frame(&frame, 0.70, 0.90);
    initialize_region(&region);

    region.tectonic_setting = CAMEFF_TECTONIC_CRUSTAL;
    region.crustal_fault_affinity = 1.0;
    region.fault_map_confidence = 0.25;

    frame.signals[
        CAMEFF_SIGNAL_SPATIAL_CONCENTRATION
    ].value = 1.0;

    CHECK(
        cameff_classify_patterns(
            &frame,
            &region,
            &result
        ) == CAMEFF_STATUS_OK
    );

    CHECK(
        result.scores[
            CAMEFF_PATTERN_CRUSTAL_FAULT_ACTIVATION
        ].membership >
        result.scores[
            CAMEFF_PATTERN_SUBDUCTION_PREPARATION
        ].membership
    );

    return 0;
}

static int test_low_quality_increases_unknown(void) {
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_pattern_result_t low_quality;
    cameff_pattern_result_t high_quality;

    initialize_frame(&frame, 0.40, 0.20);
    initialize_region(&region);

    region.catalog_completeness = 0.10;
    region.station_coverage = 0.10;
    region.regional_prior_confidence = 0.10;

    frame.data_confidence = 0.10;
    frame.signals[
        CAMEFF_SIGNAL_CATALOG_COMPLETENESS
    ].value = 0.10;

    CHECK(
        cameff_classify_patterns(
            &frame,
            &region,
            &low_quality
        ) == CAMEFF_STATUS_OK
    );

    initialize_frame(&frame, 0.40, 0.90);
    initialize_region(&region);

    CHECK(
        cameff_classify_patterns(
            &frame,
            &region,
            &high_quality
        ) == CAMEFF_STATUS_OK
    );

    CHECK(
        low_quality.scores[
            CAMEFF_PATTERN_UNKNOWN
        ].membership >
        high_quality.scores[
            CAMEFF_PATTERN_UNKNOWN
        ].membership
    );

    CHECK(
        low_quality.classification_confidence <
        high_quality.classification_confidence
    );

    return 0;
}

static int test_invalid_arguments(void) {
    cameff_signal_frame_t frame;
    cameff_region_profile_t region;
    cameff_pattern_result_t result;

    initialize_frame(&frame, 0.50, 0.80);
    initialize_region(&region);

    CHECK(
        cameff_classify_patterns(
            NULL,
            &region,
            &result
        ) == CAMEFF_STATUS_INVALID_ARGUMENT
    );

    CHECK(
        cameff_classify_patterns(
            &frame,
            NULL,
            &result
        ) == CAMEFF_STATUS_INVALID_ARGUMENT
    );

    CHECK(
        cameff_classify_patterns(
            &frame,
            &region,
            NULL
        ) == CAMEFF_STATUS_INVALID_ARGUMENT
    );

    return 0;
}

int main(void) {
    CHECK(test_memberships_are_normalized() == 0);
    CHECK(test_quiet_signals_favor_background() == 0);
    CHECK(test_subduction_profile_favors_subduction() == 0);
    CHECK(test_crustal_profile_favors_crustal_pattern() == 0);
    CHECK(test_low_quality_increases_unknown() == 0);
    CHECK(test_invalid_arguments() == 0);

    CHECK(
        strcmp(
            cameff_pattern_name(
                CAMEFF_PATTERN_SEISMIC_CASCADE
            ),
            "SEISMIC_CASCADE"
        ) == 0
    );

    puts("pattern classifier tests passed");
    return 0;
}