#ifndef CAMEFF_FUSION_H
#define CAMEFF_FUSION_H

#include <stddef.h>

#include "cameff/expert.h"
#include "cameff/pattern.h"

cameff_status_t cameff_fuse_expert_results(
    const cameff_expert_result_t *expert_results,
    size_t expert_result_count,
    const cameff_pattern_result_t *patterns,
    double minimum_confidence,
    double watch_threshold,
    double elevated_threshold,
    cameff_multi_expert_result_t *result
);

#endif