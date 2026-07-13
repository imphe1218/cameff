#include "earthquake_catalog.h"

#include <stdio.h>
#include <string.h>


void catalog_initialize(
        EarthquakeCatalog *catalog
)
{
    if(catalog == NULL)
        return;


    catalog->count = 0;
}



int catalog_load_csv(
        const char *filename,
        EarthquakeCatalog *catalog
)
{

    if(filename == NULL || catalog == NULL)
        return -1;


    FILE *file = fopen(filename,"r");


    if(file == NULL)
        return -1;


    char line[256];


    /*
       Skip CSV header
    */

    if (fgets(line, sizeof(line), file) == NULL)
    {
        fclose(file);
        return 0;
    }



    while(fgets(line,sizeof(line),file))
    {

        if(catalog->count >= MAX_EVENTS)
            break;


        EarthquakeEvent *event =
            &catalog->events[catalog->count];


        /*
          CSV parsing will be implemented
          in next step.

          For now:
          reserve event slot.
        */


        memset(event,0,sizeof(EarthquakeEvent));


        catalog->count++;

    }


    fclose(file);


    return catalog->count;

}