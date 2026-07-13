#ifndef CAMEFF_HINDCAST_EVALUATOR_H
#define CAMEFF_HINDCAST_EVALUATOR_H

#include "../data/earthquake_catalog.h"
#include "../data/earthquake_event.h"

#include <stddef.h>

#define CAMEFF_EXPERT_NAME_CAPACITY 64

typedef enum
{
    CAMEFF_EVALUATOR_OK = 0,
    CAMEFF_EVALUATOR_INVALID_ARGUMENT = -1,
    CAMEFF_EVALUATOR_EXECUTION_FAILED = -2,
    CAMEFF_EVALUATOR_INVALID_OUTPUT = -3
} CameffEvaluatorStatus;

typedef struct
{
    double hazard_score;
    double confidence;

    char dominant_pattern[
        CAMEFF_EXPERT_NAME_CAPACITY
    ];

    char dominant_expert[
        CAMEFF_EXPERT_NAME_CAPACITY
    ];
} CameffEvaluationOutput;

/*
 * The evaluator may use target_context only for the
 * predefined analysis location.
 *
 * It must not use target magnitude, depth, timestamp-derived
 * post-event information, or other future knowledge as
 * precursor evidence.
 */
typedef int (*CameffHindcastEvaluator)(
    const EarthquakeCatalog *evidence_catalog,
    const EarthquakeEvent *target_context,
    CameffEvaluationOutput *output,
    void *user_context
);

void cameff_evaluation_output_initialize(
    CameffEvaluationOutput *output
);

int cameff_evaluation_output_validate(
    const CameffEvaluationOutput *output
);

#endif