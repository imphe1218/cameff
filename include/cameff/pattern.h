#ifndef CAMEFF_PATTERN_H
#define CAMEFF_PATTERN_H

#include <stddef.h>

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

#endif
