#include "cameff/cameff.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define TEST_TOLERANCE 1e-12

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, \
                "check failed: %s at line %d\n", \
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

static void initialize_result(
    cameff_expert_result_t *result,
    const char *name,
    cameff_expert_status_t status,
    double routing_strength,
    double confidence,
    double hazard,
    double preparation,
    double cascade
) {
    (void)memset(result, 0, sizeof(*result));

    (void)snprintf(
        result->expert_name,
        sizeof(result->expert_name),
        "%s",
        name
    );

    result->status = status;
    result->routing_strength = routing_strength;
    result->confidence = confidence;
    result->hazard_evidence = hazard;
    result->preparation_evidence = preparation;
    result->cascade_evidence = cascade;
}

static void initialize_patterns(
    cameff_pattern_result_t *patterns,
    double confidence
) {
    (void)memset(patterns, 0, sizeof(*patterns));

    patterns->count = CAMEFF_PATTERN_COUNT;
    patterns->dominant_pattern =
        CAMEFF_PATTERN_SUBDUCTION_PREPARATION;

    patterns->classification_confidence =
        confidence;
}

static int test_weighted_fusion(void) {
    cameff_expert_result_t experts[2];
    cameff_pattern_result_t patterns;
    cameff_multi_expert_result_t fused;
    double expected_hazard;
    double expected_preparation;
    double expected_cascade;

    initialize_result(
        &experts[0],
        "subduction",
        CAMEFF_EXPERT_OK,
        0.80,
        0.90,
        0.80,
        0.85,
        0.10
    );

    initialize_result(
        &experts[1],
        "cascade",
        CAMEFF_EXPERT_OK,
        0.40,
        0.50,
        0.50,
        0.20,
        0.90
    );

    initialize_patterns(&patterns, 1.0);

    /*
     * Effective weights:
     *
     * subduction = 0.80 * 0.90 = 0.72
     * cascade    = 0.40 * 0.50 = 0.20
     */
    expected_hazard =
        ((0.72 * 0.80) + (0.20 * 0.50)) /
        (0.72 + 0.20);

    expected_preparation =
        ((0.72 * 0.85) + (0.20 * 0.20)) /
        (0.72 + 0.20);

    expected_cascade =
        ((0.72 * 0.10) + (0.20 * 0.90)) /
        (0.72 + 0.20);

    CHECK(
        cameff_fuse_expert_results(
            experts,
            2U,
            &patterns,
            0.25,
            0.45,
            0.70,
            &fused
        ) == CAMEFF_STATUS_OK
    );

    CHECK_CLOSE(
        fused.hazard_evidence,
        expected_hazard,
        TEST_TOLERANCE
    );

    CHECK_CLOSE(
        fused.preparation_evidence,
        expected_preparation,
        TEST_TOLERANCE
    );

    CHECK_CLOSE(
        fused.cascade_evidence,
        expected_cascade,
        TEST_TOLERANCE
    );

    CHECK(
        fused.decision ==
        CAMEFF_DECISION_ELEVATED
    );

    CHECK(
        fused.dominant_pattern ==
        CAMEFF_PATTERN_SUBDUCTION_PREPARATION
    );

    return 0;
}

static int test_abstention_is_excluded(void) {
    cameff_expert_result_t experts[2];
    cameff_pattern_result_t patterns;
    cameff_multi_expert_result_t fused;

    initialize_result(
        &experts[0],
        "baseline",
        CAMEFF_EXPERT_OK,
        1.0,
        0.80,
        0.60,
        0.00,
        0.00
    );

    initialize_result(
        &experts[1],
        "crustal-fault",
        CAMEFF_EXPERT_ABSTAIN,
        0.10,
        1.00,
        1.00,
        1.00,
        1.00
    );

    initialize_patterns(&patterns, 1.0);

    CHECK(
        cameff_fuse_expert_results(
            experts,
            2U,
            &patterns,
            0.25,
            0.45,
            0.70,
            &fused
        ) == CAMEFF_STATUS_OK
    );

    CHECK_CLOSE(
        fused.hazard_evidence,
        0.60,
        TEST_TOLERANCE
    );

    CHECK_CLOSE(
        fused.preparation_evidence,
        0.00,
        TEST_TOLERANCE
    );

    CHECK_CLOSE(
        fused.cascade_evidence,
        0.00,
        TEST_TOLERANCE
    );

    CHECK(
        fused.expert_results[1].status ==
        CAMEFF_EXPERT_ABSTAIN
    );

    return 0;
}

static int test_no_active_experts(void) {
    cameff_expert_result_t experts[2];
    cameff_pattern_result_t patterns;
    cameff_multi_expert_result_t fused;

    initialize_result(
        &experts[0],
        "subduction",
        CAMEFF_EXPERT_ABSTAIN,
        0.10,
        0.90,
        0.90,
        0.90,
        0.00
    );

    initialize_result(
        &experts[1],
        "cascade",
        CAMEFF_EXPERT_INSUFFICIENT_DATA,
        0.40,
        0.10,
        0.80,
        0.10,
        0.80
    );

    initialize_patterns(&patterns, 0.80);

    CHECK(
        cameff_fuse_expert_results(
            experts,
            2U,
            &patterns,
            0.25,
            0.45,
            0.70,
            &fused
        ) == CAMEFF_STATUS_OK
    );

    CHECK_CLOSE(
        fused.hazard_evidence,
        0.0,
        TEST_TOLERANCE
    );

    CHECK_CLOSE(
        fused.fused_confidence,
        0.0,
        TEST_TOLERANCE
    );

    CHECK(
        fused.decision ==
        CAMEFF_DECISION_INSUFFICIENT_DATA
    );

    return 0;
}

static int test_low_classification_confidence(void) {
    cameff_expert_result_t expert;
    cameff_pattern_result_t patterns;
    cameff_multi_expert_result_t fused;

    initialize_result(
        &expert,
        "subduction",
        CAMEFF_EXPERT_OK,
        1.0,
        0.90,
        0.90,
        0.90,
        0.00
    );

    initialize_patterns(&patterns, 0.20);

    CHECK(
        cameff_fuse_expert_results(
            &expert,
            1U,
            &patterns,
            0.25,
            0.45,
            0.70,
            &fused
        ) == CAMEFF_STATUS_OK
    );

    CHECK_CLOSE(
        fused.fused_confidence,
        0.18,
        TEST_TOLERANCE
    );

    CHECK(
        fused.decision ==
        CAMEFF_DECISION_INSUFFICIENT_DATA
    );

    return 0;
}

static int test_invalid_thresholds(void) {
    cameff_expert_result_t expert;
    cameff_multi_expert_result_t fused;

    initialize_result(
        &expert,
        "baseline",
        CAMEFF_EXPERT_OK,
        1.0,
        1.0,
        0.50,
        0.00,
        0.00
    );

    CHECK(
        cameff_fuse_expert_results(
            &expert,
            1U,
            NULL,
            0.25,
            0.80,
            0.70,
            &fused
        ) == CAMEFF_STATUS_INVALID_ARGUMENT
    );

    return 0;
}

int main(void) {
    CHECK(test_weighted_fusion() == 0);
    CHECK(test_abstention_is_excluded() == 0);
    CHECK(test_no_active_experts() == 0);
    CHECK(test_low_classification_confidence() == 0);
    CHECK(test_invalid_thresholds() == 0);

    puts("expert fusion tests passed");
    return 0;
}