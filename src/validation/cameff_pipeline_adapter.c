#include "cameff_pipeline_adapter.h"

#include "cameff/cameff.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int context_is_valid(
    const CameffPipelineAdapterContext *context
)
{
    if (context == NULL)
    {
        return 0;
    }

    if (context->analysis_latitude < -90.0 ||
        context->analysis_latitude > 90.0)
    {
        return 0;
    }

    if (context->analysis_longitude < -180.0 ||
        context->analysis_longitude > 180.0)
    {
        return 0;
    }

    if (context->analysis_radius_km <= 0.0)
    {
        return 0;
    }

    if (context->lookback_days == 0U)
    {
        return 0;
    }

    if (context->cutoff_epoch_seconds <= 0)
    {
        return 0;
    }

    return 1;
}

static int convert_catalog(
    const EarthquakeCatalog *source,
    cameff_catalog_t *destination
)
{
    size_t index;

    if (source == NULL || destination == NULL)
    {
        return 0;
    }

    destination->items = NULL;
    destination->count = 0U;
    destination->capacity = 0U;

    if (source->count == 0U)
    {
        return 1;
    }

    destination->items = calloc(
        source->count,
        sizeof(*destination->items)
    );

    if (destination->items == NULL)
    {
        return 0;
    }

    destination->count = source->count;
    destination->capacity = source->count;

    for (index = 0U; index < source->count; index++)
    {
        const EarthquakeEvent *source_event;
        cameff_event_t *destination_event;

        source_event = &source->events[index];
        destination_event =
            &destination->items[index];

        (void)snprintf(
            destination_event->id,
            sizeof(destination_event->id),
            "hindcast-%zu",
            index
        );

        (void)snprintf(
            destination_event->source,
            sizeof(destination_event->source),
            "%s",
            "historical"
        );

        destination_event->epoch_seconds =
            (int64_t)source_event->timestamp;

        destination_event->latitude_deg =
            source_event->latitude;

        destination_event->longitude_deg =
            source_event->longitude;

        destination_event->depth_km =
            source_event->depth_km;

        destination_event->magnitude =
            source_event->magnitude;
    }

    return 1;
}

static void release_catalog(
    cameff_catalog_t *catalog
)
{
    if (catalog == NULL)
    {
        return;
    }

    free(catalog->items);

    catalog->items = NULL;
    catalog->count = 0U;
    catalog->capacity = 0U;
}

static const char *find_dominant_expert(
    const cameff_multi_expert_result_t *fusion
)
{
    const char *dominant_name;
    double dominant_contribution;
    size_t index;

    if (fusion == NULL ||
        fusion->expert_result_count == 0U)
    {
        return NULL;
    }

    dominant_name = NULL;
    dominant_contribution = -1.0;

    for (index = 0U;
         index < fusion->expert_result_count;
         index++)
    {
        const cameff_expert_result_t *expert;
        double contribution;

        expert = &fusion->expert_results[index];

        if (expert->status != CAMEFF_EXPERT_OK)
        {
            continue;
        }

        contribution =
            expert->routing_strength *
            expert->confidence;

        if (contribution > dominant_contribution)
        {
            dominant_contribution = contribution;
            dominant_name = expert->expert_name;
        }
    }

    return dominant_name;
}

void cameff_pipeline_adapter_context_initialize(
    CameffPipelineAdapterContext *context
)
{
    if (context == NULL)
    {
        return;
    }

    (void)memset(context, 0, sizeof(*context));

    cameff_config_default(&context->config);

    cameff_evaluation_options_default(
        &context->options
    );

    context->analysis_latitude = 0.0;
    context->analysis_longitude = 0.0;
    context->analysis_radius_km = 100.0;
    context->lookback_days = 365U;
    context->cutoff_epoch_seconds = 0;
}

int cameff_pipeline_evaluator(
    const EarthquakeCatalog *evidence_catalog,
    const EarthquakeEvent *target_context,
    CameffEvaluationOutput *output,
    void *user_context
)
{
    CameffPipelineAdapterContext *context;
    cameff_catalog_t cameff_catalog;
    cameff_analysis_window_t window;
    cameff_signal_frame_t frame;
    cameff_evaluation_result_t evaluation;
    cameff_status_t status;
    const char *pattern_name;
    const char *expert_name;

    /*
     * Deliberately unused.
     *
     * This prevents target magnitude, target depth, or other
     * post-event information from entering the signal pipeline.
     */
    (void)target_context;

    if (evidence_catalog == NULL ||
        output == NULL ||
        user_context == NULL)
    {
        return CAMEFF_EVALUATOR_INVALID_ARGUMENT;
    }

    context =
        (CameffPipelineAdapterContext *)user_context;

    if (!context_is_valid(context))
    {
        return CAMEFF_EVALUATOR_INVALID_ARGUMENT;
    }

    cameff_evaluation_output_initialize(output);

    if (!convert_catalog(
            evidence_catalog,
            &cameff_catalog))
    {
        return CAMEFF_EVALUATOR_EXECUTION_FAILED;
    }

    (void)memset(&window, 0, sizeof(window));

    window.center_latitude_deg =
        context->analysis_latitude;

    window.center_longitude_deg =
        context->analysis_longitude;

    window.radius_km =
        context->analysis_radius_km;

    window.lookback_days =
        context->lookback_days;

    /*
     * Use the actual hindcast cutoff supplied by the
     * experiment context.
     *
     * Do not derive this from the last evidence event.
     */
    window.cutoff_epoch_seconds =
        context->cutoff_epoch_seconds;

    status = cameff_extract_signal_frame(
        &cameff_catalog,
        &window,
        &context->config,
        &frame
    );

    if (status != CAMEFF_STATUS_OK)
    {
        release_catalog(&cameff_catalog);

        return CAMEFF_EVALUATOR_EXECUTION_FAILED;
    }

    output->signal_count =
        CAMEFF_SIGNAL_COUNT;

    (void)memcpy(
        output->signals,
        frame.signals,
        sizeof(output->signals)
    );

    status = cameff_evaluate_experts(
        &frame,
        &context->config,
        &context->region,
        &context->options,
        &evaluation
    );

    release_catalog(&cameff_catalog);

    if (status != CAMEFF_STATUS_OK)
    {
        return CAMEFF_EVALUATOR_EXECUTION_FAILED;
    }

    output->hazard_score =
        evaluation.fusion.hazard_evidence;

    output->confidence =
        evaluation.fusion.fused_confidence;

    pattern_name = cameff_pattern_name(
        evaluation.fusion.dominant_pattern
    );

    expert_name = find_dominant_expert(
        &evaluation.fusion
    );

    if (pattern_name == NULL ||
        expert_name == NULL)
    {
        return CAMEFF_EVALUATOR_INVALID_OUTPUT;
    }

    (void)snprintf(
        output->dominant_pattern,
        sizeof(output->dominant_pattern),
        "%s",
        pattern_name
    );

    (void)snprintf(
        output->dominant_expert,
        sizeof(output->dominant_expert),
        "%s",
        expert_name
    );

    return CAMEFF_EVALUATOR_OK;
}