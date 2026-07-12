#include "cameff/cameff.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed: %s at line %d\n", #condition, __LINE__); \
        return 1; \
    } \
} while (0)

static double test_applicability(
    const cameff_signal_frame_t *frame,
    const cameff_region_profile_t *region,
    const cameff_pattern_result_t *patterns
) {
    (void)frame;
    (void)region;
    (void)patterns;
    return 1.0;
}

static cameff_status_t test_evaluate(
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
    result->status = CAMEFF_EXPERT_OK;
    result->applicability = 1.0;
    result->confidence = 1.0;
    return CAMEFF_STATUS_OK;
}

int main(void) {
    cameff_region_profile_t region;
    cameff_pattern_result_t patterns;
    cameff_expert_t expert;
    cameff_expert_result_t result;
    cameff_multi_expert_result_t fused;

    (void)memset(&region, 0, sizeof(region));
    (void)memset(&patterns, 0, sizeof(patterns));
    (void)memset(&result, 0, sizeof(result));
    (void)memset(&fused, 0, sizeof(fused));

    region.tectonic_setting = CAMEFF_TECTONIC_SUBDUCTION;
    region.subduction_affinity = 1.0;
    patterns.count = CAMEFF_PATTERN_COUNT;
    patterns.dominant_pattern = CAMEFF_PATTERN_SUBDUCTION_PREPARATION;

    expert = (cameff_expert_t){
        "contract-test",
        CAMEFF_PATTERN_SUBDUCTION_PREPARATION,
        test_applicability,
        test_evaluate
    };

    CHECK(CAMEFF_PATTERN_COUNT == 7U);
    CHECK(CAMEFF_MAX_EXPERTS >= 4U);
    CHECK(region.tectonic_setting == CAMEFF_TECTONIC_SUBDUCTION);
    CHECK(patterns.count == CAMEFF_PATTERN_COUNT);
    CHECK(expert.applicability(NULL, &region, &patterns) == 1.0);
    CHECK(expert.evaluate(NULL, NULL, &region, &patterns, &result) == CAMEFF_STATUS_OK);
    CHECK(result.status == CAMEFF_EXPERT_OK);

    fused.dominant_pattern = patterns.dominant_pattern;
    fused.decision = CAMEFF_DECISION_INSUFFICIENT_DATA;
    CHECK(fused.dominant_pattern == CAMEFF_PATTERN_SUBDUCTION_PREPARATION);

    puts("milestone 2 contract tests passed");
    return 0;
}
