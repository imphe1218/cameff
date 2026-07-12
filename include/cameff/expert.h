#ifndef CAMEFF_EXPERT_H
#define CAMEFF_EXPERT_H

#include <stddef.h>

#include "cameff/config.h"
#include "cameff/pattern.h"
#include "cameff/region.h"
#include "cameff/types.h"

#define CAMEFF_MAX_EXPERTS 8U
#define CAMEFF_MAX_EXPERT_NAME 48U
#define CAMEFF_MAX_EXPLANATION 256U

typedef enum {
    CAMEFF_EXPERT_OK = 0,
    CAMEFF_EXPERT_ABSTAIN = 1,
    CAMEFF_EXPERT_INSUFFICIENT_DATA = 2,
    CAMEFF_EXPERT_ERROR = 3
} cameff_expert_status_t;

typedef struct {
    char expert_name[CAMEFF_MAX_EXPERT_NAME];

    double hazard_evidence;
    double preparation_evidence;
    double cascade_evidence;

    double confidence;
    double applicability;
    cameff_decision_level_t decision;

    cameff_expert_status_t status;
    char explanation[CAMEFF_MAX_EXPLANATION];
} cameff_expert_result_t;

typedef double (*cameff_expert_applicability_fn)(
    const cameff_signal_frame_t *frame,
    const cameff_region_profile_t *region,
    const cameff_pattern_result_t *patterns
);

typedef cameff_status_t (*cameff_expert_evaluate_fn)(
    const cameff_signal_frame_t *frame,
    const cameff_config_t *config,
    const cameff_region_profile_t *region,
    const cameff_pattern_result_t *patterns,
    cameff_expert_result_t *result
);

typedef struct {
    const char *name;
    cameff_pattern_id_t primary_pattern;
    cameff_expert_applicability_fn applicability;
    cameff_expert_evaluate_fn evaluate;
} cameff_expert_t;

typedef struct {
    double hazard_evidence;
    double preparation_evidence;
    double cascade_evidence;

    double fused_confidence;
    double expert_coverage;

    cameff_pattern_id_t dominant_pattern;
    cameff_decision_level_t decision;

    cameff_expert_result_t expert_results[CAMEFF_MAX_EXPERTS];
    size_t expert_result_count;
} cameff_multi_expert_result_t;

#endif
