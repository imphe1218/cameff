#ifndef HINDCAST_H
#define HINDCAST_H

#include "../data/earthquake_catalog.h"
#include "../data/earthquake_event.h"

#include <stddef.h>
#include <time.h>

typedef enum
{
    CAMEFF_HINDCAST_OK = 0,
    CAMEFF_HINDCAST_INVALID_ARGUMENT = -1,
    CAMEFF_HINDCAST_INVALID_TARGET = -2,
    CAMEFF_HINDCAST_INVALID_TIME_WINDOW = -3,
    CAMEFF_HINDCAST_INVALID_RADIUS = -4,
    CAMEFF_HINDCAST_FILTER_FAILED = -5,
    CAMEFF_HINDCAST_NO_EVIDENCE = -6,
    CAMEFF_HINDCAST_NOT_EVALUATED = 1
} CameffHindcastStatus;

typedef struct
{
    EarthquakeEvent target;

    time_t observation_start;
    time_t cutoff_time;

    double analysis_radius_km;

    const EarthquakeCatalog *source_catalog;
} HindcastExperiment;

typedef struct
{
    CameffHindcastStatus status;

    size_t evidence_event_count;

    time_t observation_start;
    time_t cutoff_time;

    double hazard_score;
    double confidence;

    char dominant_expert[64];
} HindcastResult;

void hindcast_experiment_initialize(
    HindcastExperiment *experiment
);

void hindcast_result_initialize(
    HindcastResult *result
);

int hindcast_experiment_validate(
    const HindcastExperiment *experiment
);

int run_hindcast(
    const HindcastExperiment *experiment,
    HindcastResult *result
);

#endif