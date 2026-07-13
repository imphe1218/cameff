#include "hindcast.h"

#include "../data/earthquake_catalog_filter.h"

#include <string.h>

static int target_is_valid(
    const EarthquakeEvent *target
)
{
    if (target == NULL)
    {
        return 0;
    }

    if (target->latitude < -90.0 ||
        target->latitude > 90.0)
    {
        return 0;
    }

    if (target->longitude < -180.0 ||
        target->longitude > 180.0)
    {
        return 0;
    }

    if (target->depth_km < 0.0)
    {
        return 0;
    }

    if (target->magnitude < -2.0 ||
        target->magnitude > 10.5)
    {
        return 0;
    }

    if (target->timestamp <= (time_t)0)
    {
        return 0;
    }

    return 1;
}

void hindcast_experiment_initialize(
    HindcastExperiment *experiment
)
{
    if (experiment == NULL)
    {
        return;
    }

    initialize_earthquake_event(
        &experiment->target
    );

    experiment->observation_start = (time_t)0;
    experiment->cutoff_time = (time_t)0;
    experiment->analysis_radius_km = 0.0;
    experiment->source_catalog = NULL;

    experiment->evaluator = NULL;
    experiment->evaluator_context = NULL;
}

void hindcast_result_initialize(
    HindcastResult *result
)
{
    size_t index;

    if (result == NULL)
    {
        return;
    }

    result->status =
        CAMEFF_HINDCAST_NOT_EVALUATED;

    result->evidence_event_count = 0U;

    result->observation_start = (time_t)0;
    result->cutoff_time = (time_t)0;

    result->hazard_score = 0.0;
    result->confidence = 0.0;

    result->signal_count = 0U;

    for (index = 0U;
         index < CAMEFF_SIGNAL_COUNT;
         index++)
    {
        result->signals[index].value = 0.0;
        result->signals[index].confidence = 0.0;
        result->signals[index].supporting_events = 0U;
        result->signals[index].available = 0;
    }

    result->dominant_pattern[0] = '\0';
    result->dominant_expert[0] = '\0';
}

int hindcast_experiment_validate(
    const HindcastExperiment *experiment
)
{
    if (experiment == NULL)
    {
        return CAMEFF_HINDCAST_INVALID_ARGUMENT;
    }

    if (!target_is_valid(&experiment->target))
    {
        return CAMEFF_HINDCAST_INVALID_TARGET;
    }

    if (experiment->source_catalog == NULL)
    {
        return CAMEFF_HINDCAST_INVALID_ARGUMENT;
    }

    if (experiment->observation_start >=
        experiment->cutoff_time)
    {
        return CAMEFF_HINDCAST_INVALID_TIME_WINDOW;
    }

    /*
     * The cutoff must not be later than the target.
     *
     * Equal is allowed because the target timestamp itself
     * remains excluded by the half-open filter:
     *
     *     event.timestamp < cutoff_time
     */
    if (experiment->cutoff_time >
        experiment->target.timestamp)
    {
        return CAMEFF_HINDCAST_INVALID_TIME_WINDOW;
    }

    if (experiment->analysis_radius_km <= 0.0)
    {
        return CAMEFF_HINDCAST_INVALID_RADIUS;
    }

    return CAMEFF_HINDCAST_OK;
}

int run_hindcast(
    const HindcastExperiment *experiment,
    HindcastResult *result
)
{
    EarthquakeCatalog evidence_catalog;
    CameffEvaluationOutput evaluation_output;

    int validation_status;
    int filter_status;
    int evaluator_status;
    int output_status;

    if (result == NULL)
    {
        return CAMEFF_HINDCAST_INVALID_ARGUMENT;
    }

    hindcast_result_initialize(result);

    validation_status =
        hindcast_experiment_validate(experiment);

    if (validation_status != CAMEFF_HINDCAST_OK)
    {
        result->status =
            (CameffHindcastStatus)validation_status;

        return validation_status;
    }

    result->observation_start =
        experiment->observation_start;

    result->cutoff_time =
        experiment->cutoff_time;

    filter_status =
        catalog_filter_hindcast_window(
            experiment->source_catalog,
            experiment->observation_start,
            experiment->cutoff_time,
            experiment->target.latitude,
            experiment->target.longitude,
            experiment->analysis_radius_km,
            &evidence_catalog
        );

    if (filter_status !=
        CAMEFF_CATALOG_FILTER_OK)
    {
        result->status =
            CAMEFF_HINDCAST_FILTER_FAILED;

        return CAMEFF_HINDCAST_FILTER_FAILED;
    }

    result->evidence_event_count =
        evidence_catalog.count;

    if (evidence_catalog.count == 0)
    {
        result->status =
            CAMEFF_HINDCAST_NO_EVIDENCE;

        return CAMEFF_HINDCAST_NO_EVIDENCE;
    }

    /*
     * A missing evaluator means that the historical evidence
     * window was constructed successfully, but no expert
     * pipeline has been connected.
     */
    if (experiment->evaluator == NULL)
    {
        result->status =
            CAMEFF_HINDCAST_NOT_EVALUATED;

        return CAMEFF_HINDCAST_NOT_EVALUATED;
    }

    cameff_evaluation_output_initialize(
        &evaluation_output
    );

    /*
     * The evaluator may use the target only as predefined
     * geographic context.
     *
     * It must not use the target magnitude, depth, or other
     * post-event knowledge as precursor evidence.
     */
    evaluator_status =
        experiment->evaluator(
            &evidence_catalog,
            &experiment->target,
            &evaluation_output,
            experiment->evaluator_context
        );

    if (evaluator_status !=
        CAMEFF_EVALUATOR_OK)
    {
        result->status =
            CAMEFF_HINDCAST_EVALUATION_FAILED;

        return CAMEFF_HINDCAST_EVALUATION_FAILED;
    }

    output_status =
        cameff_evaluation_output_validate(
            &evaluation_output
        );

    if (output_status !=
        CAMEFF_EVALUATOR_OK)
    {
        result->status =
            CAMEFF_HINDCAST_EVALUATION_FAILED;

        return CAMEFF_HINDCAST_EVALUATION_FAILED;
    }

    result->hazard_score =
        evaluation_output.hazard_score;

    result->confidence =
        evaluation_output.confidence;

    result->signal_count =
    evaluation_output.signal_count;

    memcpy(
        result->signals,
        evaluation_output.signals,
        sizeof(result->signals)
    );

    memcpy(
        result->dominant_pattern,
        evaluation_output.dominant_pattern,
        sizeof(result->dominant_pattern)
    );

    result->dominant_pattern[
        sizeof(result->dominant_pattern) - 1
    ] = '\0';

    memcpy(
        result->dominant_expert,
        evaluation_output.dominant_expert,
        sizeof(result->dominant_expert)
    );

    result->dominant_expert[
        sizeof(result->dominant_expert) - 1
    ] = '\0';

    result->status = CAMEFF_HINDCAST_OK;

    return CAMEFF_HINDCAST_OK;
}