#ifndef EARTHQUAKE_CATALOG_H
#define EARTHQUAKE_CATALOG_H

#include "earthquake_event.h"

#include <stddef.h>

#define CAMEFF_CATALOG_MAX_EVENTS 10000

typedef enum
{
    CAMEFF_CATALOG_OK = 0,
    CAMEFF_CATALOG_INVALID_ARGUMENT = -1,
    CAMEFF_CATALOG_OPEN_FAILED = -2,
    CAMEFF_CATALOG_EMPTY_FILE = -3,
    CAMEFF_CATALOG_INVALID_HEADER = -4,
    CAMEFF_CATALOG_INVALID_ROW = -5,
    CAMEFF_CATALOG_CAPACITY_EXCEEDED = -6
} CameffCatalogStatus;

typedef struct
{
    EarthquakeEvent events[CAMEFF_CATALOG_MAX_EVENTS];
    size_t count;
} EarthquakeCatalog;

void catalog_initialize(EarthquakeCatalog *catalog);

int catalog_load_csv(
    const char *filename,
    EarthquakeCatalog *catalog
);

#endif