#include "earthquake_event.h"


void initialize_earthquake_event(
        EarthquakeEvent *event
)
{
    if(event == NULL)
        return;


    event->region[0] = '\0';

    event->latitude = 0.0;
    event->longitude = 0.0;

    event->depth_km = 0.0;

    event->magnitude = 0.0;

    event->timestamp = 0;
}