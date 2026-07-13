#include "../../src/data/earthquake_catalog.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#ifndef CAMEFF_TEST_SOURCE_DIR
#error "CAMEFF_TEST_SOURCE_DIR is not defined"
#endif

static int approximately_equal(
    double left,
    double right
)
{
    return fabs(left - right) < 0.000001;
}

static void test_catalog_initialization(void)
{
    EarthquakeCatalog catalog;

    catalog.count = 123;

    catalog_initialize(&catalog);

    assert(catalog.count == 0);
}

static void test_valid_catalog_loading(void)
{
    EarthquakeCatalog catalog;
    int status;

    status = catalog_load_csv(
        CAMEFF_TEST_SOURCE_DIR
        "/tests/data/fixtures/valid_catalog.csv",
        &catalog
    );

    assert(status == CAMEFF_CATALOG_OK);
    assert(catalog.count == 3);

    assert(approximately_equal(
        catalog.events[0].latitude,
        38.435
    ));

    assert(approximately_equal(
        catalog.events[2].magnitude,
        9.1
    ));

    assert(strcmp(
        catalog.events[2].region,
        "Near the east coast of Honshu, Japan"
    ) == 0);

    /*
     * 2011-03-11T05:46:24Z
     */
    assert(
        catalog.events[2].timestamp ==
        (time_t)1299822384
    );
}

static void test_invalid_catalog_rejection(void)
{
    EarthquakeCatalog catalog;
    int status;

    status = catalog_load_csv(
        CAMEFF_TEST_SOURCE_DIR
        "/tests/data/fixtures/invalid_catalog.csv",
        &catalog
    );

    assert(status == CAMEFF_CATALOG_INVALID_ROW);
    assert(catalog.count == 0);
}

static void test_missing_catalog_rejection(void)
{
    EarthquakeCatalog catalog;
    int status;

    status = catalog_load_csv(
        CAMEFF_TEST_SOURCE_DIR
        "/tests/data/fixtures/does-not-exist.csv",
        &catalog
    );

    assert(status == CAMEFF_CATALOG_OPEN_FAILED);
    assert(catalog.count == 0);
}

int main(void)
{
    test_catalog_initialization();
    test_valid_catalog_loading();
    test_invalid_catalog_rejection();
    test_missing_catalog_rejection();

    return 0;
}