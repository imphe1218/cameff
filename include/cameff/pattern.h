#ifndef CAMEFF_PATTERN_H
#define CAMEFF_PATTERN_H

#include <stddef.h>

#include "cameff/region.h"
#include "cameff/types.h"

#define CAMEFF_PATTERN_COUNT 7U

typedef enum {
    CAMEFF_PATTERN_BACKGROUND = 0,
    CAMEFF_PATTERN_SUBDUCTION_PREPARATION = 1,
    CAMEFF_PATTERN_CRUSTAL_FAULT_ACTIVATION = 2,
    CAMEFF_PATTERN_SEISMIC_CASCADE = 3,
    CAMEFF_PATTERN_VOLCANIC_UNREST = 4,
    CAMEFF_PATTERN_TRANSFORM_FAULT_ACTIVITY = 5,
    CAMEFF_PATTERN_UNKNOWN = 6
} cameff_pattern_id_t;

typedef struct {
    cameff_pattern_id_t pattern_id;
    double membership;
    double confidence;
} cameff_pattern_score_t;

typedef struct {
    cameff_pattern_score_t scores[CAMEFF_PATTERN_COUNT];
    size_t count;

    double classification_confidence;
    cameff_pattern_id_t dominant_pattern;
} cameff_pattern_result_t;

/*
 * Produces deterministic compatibility memberships for the currently
 * supported process-pattern families.
 *
 * Memberships are normalized and sum to approximately 1.0.
 * They are not calibrated earthquake probabilities.
 */
cameff_status_t cameff_classify_patterns(
    const cameff_signal_frame_t *frame,
    const cameff_region_profile_t *region,
    cameff_pattern_result_t *result
);

const char *cameff_pattern_name(cameff_pattern_id_t pattern_id);

#endif // CAMEFF_PATTERN_H