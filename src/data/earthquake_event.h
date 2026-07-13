#ifndef EARTHQUAKE_EVENT_H
#define EARTHQUAKE_EVENT_H

#include <time.h>


typedef struct {

    char region[64];

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