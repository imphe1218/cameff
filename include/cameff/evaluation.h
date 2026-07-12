#ifndef CAMEFF_EVALUATION_H
#define CAMEFF_EVALUATION_H

#include <stddef.h>

#include "cameff/config.h"
#include "cameff/expert.h"
#include "cameff/pattern.h"
#include "cameff/region.h"
#include "cameff/types.h"

typedef struct {
    double routing_threshold;
    double minimum_fused_confidence;
    double watch_threshold;
    double elevated_threshold;
} cameff_evaluation_options_t;

typedef struct {
    cameff_pattern_result_t patterns;
    cameff_multi_expert_result_t fusion;
} cameff_evaluation_result_t;

void cameff_evaluation_options_default(
    cameff_evaluation_options_t *options
);

cameff_status_t cameff_evaluate_experts(
    const cameff_signal_frame_t *frame,
    const cameff_config_t *config,
    const cameff_region_profile_t *region,
    const cameff_evaluation_options_t *options,
    cameff_evaluation_result_t *result
);

cameff_status_t cameff_format_evaluation(
    const cameff_evaluation_result_t *result,
    char *buffer,
    size_t buffer_capacity
);

#endif