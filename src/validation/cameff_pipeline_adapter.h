#ifndef CAMEFF_PIPELINE_ADAPTER_H
#define CAMEFF_PIPELINE_ADAPTER_H

#include "hindcast_evaluator.h"

#include "cameff/config.h"
#include "cameff/evaluation.h"
#include "cameff/region.h"

#include <stdint.h>

typedef struct
{
    cameff_config_t config;
    cameff_evaluation_options_t options;
    cameff_region_profile_t region;

    double analysis_latitude;
    double analysis_longitude;

    double analysis_radius_km;
    unsigned lookback_days;
    int64_t cutoff_epoch_seconds;
    
} CameffPipelineAdapterContext;

void cameff_pipeline_adapter_context_initialize(
    CameffPipelineAdapterContext *context
);

int cameff_pipeline_evaluator(
    const EarthquakeCatalog *evidence_catalog,
    const EarthquakeEvent *target_context,
    CameffEvaluationOutput *output,
    void *user_context
);

#endif