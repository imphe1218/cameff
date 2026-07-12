#ifndef CAMEFF_EXPERT_REGISTRY_H
#define CAMEFF_EXPERT_REGISTRY_H

#include <stddef.h>

#include "cameff/expert.h"

typedef struct {
    const cameff_expert_t *experts[CAMEFF_MAX_EXPERTS];
    size_t count;
} cameff_expert_registry_t;

void cameff_expert_registry_init(
    cameff_expert_registry_t *registry
);

cameff_status_t cameff_expert_registry_register(
    cameff_expert_registry_t *registry,
    const cameff_expert_t *expert
);

size_t cameff_expert_registry_count(
    const cameff_expert_registry_t *registry
);

const cameff_expert_t *cameff_expert_registry_get(
    const cameff_expert_registry_t *registry,
    size_t index
);

double cameff_expert_routing_strength(
    const cameff_expert_t *expert,
    const cameff_signal_frame_t *frame,
    const cameff_region_profile_t *region,
    const cameff_pattern_result_t *patterns
);

cameff_status_t cameff_expert_registry_evaluate(
    const cameff_expert_registry_t *registry,
    const cameff_signal_frame_t *frame,
    const cameff_config_t *config,
    const cameff_region_profile_t *region,
    const cameff_pattern_result_t *patterns,
    double routing_threshold,
    cameff_expert_result_t *results,
    size_t result_capacity,
    size_t *result_count
);

#endif