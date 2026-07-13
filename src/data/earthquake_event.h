#ifndef EARTHQUAKE_EVENT_H
#define EARTHQUAKE_EVENT_H

#define CAMEFF_EARTHQUAKE_REGION_CAPACITY 64U

#include <time.h>


typedef struct
{
    char region[
        CAMEFF_EARTHQUAKE_REGION_CAPACITY
    ];

    double latitude;
    double longitude;
    double depth_km;
    double magnitude;

    time_t timestamp;
} EarthquakeEvent;



void initialize_earthquake_event(
        EarthquakeEvent *event
);


#endif