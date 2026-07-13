#ifndef EARTHQUAKE_CATALOG_H
#define EARTHQUAKE_CATALOG_H


#include "earthquake_event.h"


#define MAX_EVENTS 10000


typedef struct {

    EarthquakeEvent events[MAX_EVENTS];

    int count;

} EarthquakeCatalog;



int catalog_load_csv(
        const char *filename,
        EarthquakeCatalog *catalog
);


void catalog_initialize(
        EarthquakeCatalog *catalog
);


#endif