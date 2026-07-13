#include "earthquake_catalog_filter.h"

#include <math.h>
#include <stddef.h>

#define CAMEFF_EARTH_MEAN_RADIUS_KM 6371.0088
#define CAMEFF_PI 3.14159265358979323846

static int coordinates_are_valid(
    double latitude,
    double longitude
)
{
    return
        latitude >= -90.0 &&
        latitude <= 90.0 &&
        longitude >= -180.0 &&
        longitude <= 180.0;
}

static double degrees_to_radians(
    double degrees
)
{
    return degrees * CAMEFF_PI / 180.0;
}

static int append_event(
    EarthquakeCatalog *catalog,
    const EarthquakeEvent *event
)
{
    if (catalog == NULL || event == NULL)
    {
        return CAMEFF_CATALOG_FILTER_INVALID_ARGUMENT;
    }

    if (catalog->count >= CAMEFF_CATALOG_MAX_EVENTS)
    {
        return CAMEFF_CATALOG_FILTER_CAPACITY_EXCEEDED;
    }

    catalog->events[catalog->count] = *event;
    catalog->count++;

    return CAMEFF_CATALOG_FILTER_OK;
}

double earthquake_surface_distance_km(
    double latitude_a,
    double longitude_a,
    double latitude_b,
    double longitude_b
)
{
    double latitude_a_rad;
    double latitude_b_rad;
    double latitude_delta;
    double longitude_delta;
    double haversine;
    double angular_distance;

    if (!coordinates_are_valid(
            latitude_a,
            longitude_a) ||
        !coordinates_are_valid(
            latitude_b,
            longitude_b))
    {
        return -1.0;
    }

    latitude_a_rad =
        degrees_to_radians(latitude_a);

    latitude_b_rad =
        degrees_to_radians(latitude_b);

    latitude_delta =
        latitude_b_rad - latitude_a_rad;

    longitude_delta =
        degrees_to_radians(
            longitude_b - longitude_a
        );

    haversine =
        sin(latitude_delta / 2.0) *
            sin(latitude_delta / 2.0) +
        cos(latitude_a_rad) *
            cos(latitude_b_rad) *
        sin(longitude_delta / 2.0) *
            sin(longitude_delta / 2.0);

    /*
     * Protect against small floating-point drift outside [0, 1].
     */
    if (haversine < 0.0)
    {
        haversine = 0.0;
    }
    else if (haversine > 1.0)
    {
        haversine = 1.0;
    }

    angular_distance =
        2.0 *
        atan2(
            sqrt(haversine),
            sqrt(1.0 - haversine)
        );

    return
        CAMEFF_EARTH_MEAN_RADIUS_KM *
        angular_distance;
}

int catalog_filter_time_range(
    const EarthquakeCatalog *source,
    time_t start_time,
    time_t end_time,
    EarthquakeCatalog *result
)
{
    size_t index;

    if (source == NULL || result == NULL)
    {
        return CAMEFF_CATALOG_FILTER_INVALID_ARGUMENT;
    }

    if (start_time >= end_time)
    {
        return CAMEFF_CATALOG_FILTER_INVALID_TIME_RANGE;
    }

    catalog_initialize(result);

    for (index = 0; index < source->count; index++)
    {
        const EarthquakeEvent *event;
        int status;

        event = &source->events[index];

        if (event->timestamp < start_time ||
            event->timestamp >= end_time)
        {
            continue;
        }

        status = append_event(result, event);

        if (status != CAMEFF_CATALOG_FILTER_OK)
        {
            catalog_initialize(result);
            return status;
        }
    }

    return CAMEFF_CATALOG_FILTER_OK;
}

int catalog_filter_radius(
    const EarthquakeCatalog *source,
    double center_latitude,
    double center_longitude,
    double radius_km,
    EarthquakeCatalog *result
)
{
    size_t index;

    if (source == NULL || result == NULL)
    {
        return CAMEFF_CATALOG_FILTER_INVALID_ARGUMENT;
    }

    if (!coordinates_are_valid(
            center_latitude,
            center_longitude))
    {
        return CAMEFF_CATALOG_FILTER_INVALID_COORDINATES;
    }

    if (radius_km < 0.0)
    {
        return CAMEFF_CATALOG_FILTER_INVALID_RADIUS;
    }

    catalog_initialize(result);

    for (index = 0; index < source->count; index++)
    {
        const EarthquakeEvent *event;
        double distance_km;
        int status;

        event = &source->events[index];

        distance_km =
            earthquake_surface_distance_km(
                center_latitude,
                center_longitude,
                event->latitude,
                event->longitude
            );

        if (distance_km < 0.0)
        {
            catalog_initialize(result);

            return
                CAMEFF_CATALOG_FILTER_INVALID_COORDINATES;
        }

        if (distance_km > radius_km)
        {
            continue;
        }

        status = append_event(result, event);

        if (status != CAMEFF_CATALOG_FILTER_OK)
        {
            catalog_initialize(result);
            return status;
        }
    }

    return CAMEFF_CATALOG_FILTER_OK;
}

int catalog_filter_hindcast_window(
    const EarthquakeCatalog *source,
    time_t start_time,
    time_t cutoff_time,
    double center_latitude,
    double center_longitude,
    double radius_km,
    EarthquakeCatalog *result
)
{
    size_t index;

    if (source == NULL || result == NULL)
    {
        return CAMEFF_CATALOG_FILTER_INVALID_ARGUMENT;
    }

    if (start_time >= cutoff_time)
    {
        return CAMEFF_CATALOG_FILTER_INVALID_TIME_RANGE;
    }

    if (!coordinates_are_valid(
            center_latitude,
            center_longitude))
    {
        return CAMEFF_CATALOG_FILTER_INVALID_COORDINATES;
    }

    if (radius_km < 0.0)
    {
        return CAMEFF_CATALOG_FILTER_INVALID_RADIUS;
    }

    catalog_initialize(result);

    for (index = 0; index < source->count; index++)
    {
        const EarthquakeEvent *event;
        double distance_km;
        int status;

        event = &source->events[index];

        if (event->timestamp < start_time ||
            event->timestamp >= cutoff_time)
        {
            continue;
        }

        distance_km =
            earthquake_surface_distance_km(
                center_latitude,
                center_longitude,
                event->latitude,
                event->longitude
            );

        if (distance_km < 0.0)
        {
            catalog_initialize(result);

            return
                CAMEFF_CATALOG_FILTER_INVALID_COORDINATES;
        }

        if (distance_km > radius_km)
        {
            continue;
        }

        status = append_event(result, event);

        if (status != CAMEFF_CATALOG_FILTER_OK)
        {
            catalog_initialize(result);
            return status;
        }
    }

    return CAMEFF_CATALOG_FILTER_OK;
}