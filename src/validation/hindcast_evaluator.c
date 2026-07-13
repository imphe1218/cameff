#include "hindcast_evaluator.h"

#include <math.h>

void cameff_evaluation_output_initialize(
    CameffEvaluationOutput *output
)
{
    if (output == NULL)
    {
        return;
    }

    output->hazard_score = 0.0;
    output->confidence = 0.0;
    output->dominant_expert[0] = '\0';
    output->dominant_pattern[0] = '\0';
    
}

int cameff_evaluation_output_validate(
    const CameffEvaluationOutput *output
)
{
    if (output == NULL)
    {
        return CAMEFF_EVALUATOR_INVALID_ARGUMENT;
    }

    if (!isfinite(output->hazard_score) ||
        output->hazard_score < 0.0 ||
        output->hazard_score > 1.0)
    {
        return CAMEFF_EVALUATOR_INVALID_OUTPUT;
    }

    if (!isfinite(output->confidence) ||
        output->confidence < 0.0 ||
        output->confidence > 1.0)
    {
        return CAMEFF_EVALUATOR_INVALID_OUTPUT;
    }

    if (output->dominant_expert[0] == '\0')
    {
        return CAMEFF_EVALUATOR_INVALID_OUTPUT;
    }

    if (output->dominant_pattern[0] == '\0')
    {
        return CAMEFF_EVALUATOR_INVALID_OUTPUT;
    }

    return CAMEFF_EVALUATOR_OK;
}