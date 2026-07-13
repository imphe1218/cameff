#include "../../src/data/earthquake_catalog.h"

#include <assert.h>


int main()
{

    EarthquakeCatalog catalog;


    catalog_initialize(&catalog);


    assert(catalog.count == 0);


    return 0;
}