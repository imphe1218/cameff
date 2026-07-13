#ifndef HINDCAST_H
#define HINDCAST_H

#include "../data/earthquake_event.h"


typedef struct {

    EarthquakeEvent event;

    time_t cutoff_time;

    int window_days;

} HindcastExperiment;


typedef struct {

    double hazard_score;

    double confidence;

    char expert[64];

} HindcastResult;


HindcastResult run_hindcast(
        HindcastExperiment *experiment
);


#endif