#ifndef EARTHQUAKE_CATALOG_FILTER_H
#define EARTHQUAKE_CATALOG_FILTER_H

#include "earthquake_catalog.h"

#include <time.h>

typedef enum
{
    CAMEFF_CATALOG_FILTER_OK = 0,
    CAMEFF_CATALOG_FILTER_INVALID_ARGUMENT = -1,
    CAMEFF_CATALOG_FILTER_INVALID_TIME_RANGE = -2,
    CAMEFF_CATALOG_FILTER_INVALID_COORDINATES = -3,
    CAMEFF_CATALOG_FILTER_INVALID_RADIUS = -4,
    CAMEFF_CATALOG_FILTER_CAPACITY_EXCEEDED = -5
} CameffCatalogFilterStatus;

/*
 * Selects events in:
 *
 *     start_time <= event.timestamp < end_time
 */
int catalog_filter_time_range(
    const EarthquakeCatalog *source,
    time_t start_time,
    time_t end_time,
    EarthquakeCatalog *result
);

/*
 * Selects events whose epicentres are within radius_km
 * of the supplied centre.
 */
int catalog_filter_radius(
    const EarthquakeCatalog *source,
    double center_latitude,
    double center_longitude,
    double radius_km,
    EarthquakeCatalog *result
);

/*
 * Applies both temporal and spatial filtering in one pass.
 */
int catalog_filter_hindcast_window(
    const EarthquakeCatalog *source,
    time_t start_time,
    time_t cutoff_time,
    double center_latitude,
    double center_longitude,
    double radius_km,
    EarthquakeCatalog *result
);

double earthquake_surface_distance_km(
    double latitude_a,
    double longitude_a,
    double latitude_b,
    double longitude_b
);

#endif