#include "hindcast_evaluator.h"

#include <math.h>

void cameff_evaluation_output_initialize(
    CameffEvaluationOutput *output
)
{
    size_t index;

    if (output == NULL)
    {
        return;
    }

    output->hazard_score = 0.0;
    output->confidence = 0.0;

    output->signal_count = 0U;

    for (index = 0U;
         index < CAMEFF_SIGNAL_COUNT;
         index++)
    {
        output->signals[index].value = 0.0;
        output->signals[index].confidence = 0.0;
        output->signals[index].supporting_events = 0U;
        output->signals[index].available = 0;
    }

    output->dominant_pattern[0] = '\0';
    output->dominant_expert[0] = '\0';
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

    if (output->signal_count != CAMEFF_SIGNAL_COUNT)
    {
        return CAMEFF_EVALUATOR_INVALID_OUTPUT;
    }

    {
        size_t index;

        for (index = 0U;
            index < output->signal_count;
            index++)
        {
            const cameff_signal_t *signal;

            signal = &output->signals[index];

            if (!isfinite(signal->value) ||
                signal->value < 0.0 ||
                signal->value > 1.0)
            {
                return CAMEFF_EVALUATOR_INVALID_OUTPUT;
            }

            if (!isfinite(signal->confidence) ||
                signal->confidence < 0.0 ||
                signal->confidence > 1.0)
            {
                return CAMEFF_EVALUATOR_INVALID_OUTPUT;
            }

            if (signal->available != 0 &&
                signal->available != 1)
            {
                return CAMEFF_EVALUATOR_INVALID_OUTPUT;
            }
        }
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