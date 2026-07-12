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

#define CHECK_CLOSE(actual, expected, tolerance) do { \
    if (fabs((actual) - (expected)) > (tolerance)) { \
        fprintf(stderr, \
                "check failed: %s ~= %s at line %d\n", \
                #actual, #expected, __LINE__); \
        return 1; \
    } \
} while (0)

static void initialize_frame(cameff_signal_frame_t *frame) {
    size_t i;

    (void)memset(frame, 0, sizeof(*frame));

    for (i = 0U; i < CAMEFF_SIGNAL_COUNT; ++i) {
        frame->signals[i].value = 0.60;
        frame->signals[i].confidence = 0.80;
        frame->signals[i].supporting_events = 20U;
        frame->signals[i].available = 1;
    }

    frame->selected_event_count = 20U;
    frame->data_confidence = 0.80;
}

int main(void) {
    cameff_signal_frame_t frame;
    cameff_config_t config;
    cameff_assessment_t legacy;
    cameff_expert_result_t expert_result;
    const cameff_expert_t *expert;
    cameff_status_t status;

    initialize_frame(&frame);
    cameff_config_default(&config);

    legacy = cameff_assess(&frame, &config);

    expert = cameff_baseline_expert();

    CHECK(expert != NULL);
    CHECK(expert->applicability != NULL);
    CHECK(expert->evaluate != NULL);

    CHECK_CLOSE(
        expert->applicability(&frame, NULL, NULL),
        1.0,
        1e-12
    );

    status = expert->evaluate(
        &frame,
        &config,
        NULL,
        NULL,
        &expert_result
    );

    CHECK(status == CAMEFF_STATUS_OK);
    CHECK(expert_result.status == CAMEFF_EXPERT_OK);

    CHECK_CLOSE(
        expert_result.hazard_evidence,
        legacy.evidence_score,
        1e-12
    );

    CHECK_CLOSE(
        expert_result.confidence,
        legacy.confidence,
        1e-12
    );

    CHECK(
        expert_result.decision ==
        legacy.level
    );

    CHECK_CLOSE(
        expert_result.preparation_evidence,
        0.0,
        1e-12
    );

    CHECK_CLOSE(
        expert_result.cascade_evidence,
        0.0,
        1e-12
    );

    puts("baseline expert tests passed");
    return 0;
}