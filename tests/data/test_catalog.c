#include "../../src/data/earthquake_catalog.h"
#include "../../src/data/earthquake_catalog_filter.h"

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

static void test_time_range_filtering(void)
{
    EarthquakeCatalog source;
    EarthquakeCatalog result;
    int status;

    status = catalog_load_csv(
        CAMEFF_TEST_SOURCE_DIR
        "/tests/data/fixtures/valid_catalog.csv",
        &source
    );

    assert(status == CAMEFF_CATALOG_OK);
    assert(source.count == 3);

    status = catalog_filter_time_range(
        &source,
        (time_t)1299680000,
        (time_t)1299822384,
        &result
    );

    assert(
        status ==
        CAMEFF_CATALOG_FILTER_OK
    );

    /*
     * The target event occurs exactly at the end time,
     * so it must not be included.
     */
    assert(result.count == 2);

    assert(
        result.events[0].timestamp <
        (time_t)1299822384
    );

    assert(
        result.events[1].timestamp <
        (time_t)1299822384
    );
}

static void test_cutoff_prevents_future_leakage(void)
{
    EarthquakeCatalog source;
    EarthquakeCatalog result;
    int status;

    status = catalog_load_csv(
        CAMEFF_TEST_SOURCE_DIR
        "/tests/data/fixtures/valid_catalog.csv",
        &source
    );

    assert(status == CAMEFF_CATALOG_OK);

    status = catalog_filter_time_range(
        &source,
        (time_t)0,
        (time_t)1299822384,
        &result
    );

    assert(
        status ==
        CAMEFF_CATALOG_FILTER_OK
    );

    assert(result.count == 2);

    /*
     * The Mw 9.1 target event must be hidden because its
     * timestamp equals the cutoff.
     */
    assert(
        result.events[result.count - 1].magnitude <
        9.1
    );
}

static void test_surface_distance(void)
{
    double same_location_distance;
    double japan_pair_distance;

    same_location_distance =
        earthquake_surface_distance_km(
            38.297,
            142.372,
            38.297,
            142.372
        );

    assert(
        approximately_equal(
            same_location_distance,
            0.0
        )
    );

    japan_pair_distance =
        earthquake_surface_distance_km(
            38.297,
            142.372,
            38.435,
            142.842
        );

    assert(japan_pair_distance > 40.0);
    assert(japan_pair_distance < 50.0);
}

static void test_radius_filtering(void)
{
    EarthquakeCatalog source;
    EarthquakeCatalog result;
    int status;

    status = catalog_load_csv(
        CAMEFF_TEST_SOURCE_DIR
        "/tests/data/fixtures/valid_catalog.csv",
        &source
    );

    assert(status == CAMEFF_CATALOG_OK);

    status = catalog_filter_radius(
        &source,
        38.297,
        142.372,
        50.0,
        &result
    );

    assert(
        status ==
        CAMEFF_CATALOG_FILTER_OK
    );

    assert(result.count >= 1);
    assert(result.count <= 3);
}

static void test_combined_hindcast_filter(void)
{
    EarthquakeCatalog source;
    EarthquakeCatalog result;
    int status;
    size_t index;

    status = catalog_load_csv(
        CAMEFF_TEST_SOURCE_DIR
        "/tests/data/fixtures/valid_catalog.csv",
        &source
    );

    assert(status == CAMEFF_CATALOG_OK);

    status = catalog_filter_hindcast_window(
        &source,
        (time_t)1299600000,
        (time_t)1299822384,
        38.297,
        142.372,
        100.0,
        &result
    );

    assert(
        status ==
        CAMEFF_CATALOG_FILTER_OK
    );

    assert(result.count == 2);

    for (index = 0; index < result.count; index++)
    {
        double distance_km;

        assert(
            result.events[index].timestamp <
            (time_t)1299822384
        );

        distance_km =
            earthquake_surface_distance_km(
                38.297,
                142.372,
                result.events[index].latitude,
                result.events[index].longitude
            );

        assert(distance_km <= 100.0);
    }
}

static void test_invalid_filter_arguments(void)
{
    EarthquakeCatalog source;
    EarthquakeCatalog result;

    catalog_initialize(&source);

    assert(
        catalog_filter_time_range(
            &source,
            (time_t)100,
            (time_t)100,
            &result
        ) ==
        CAMEFF_CATALOG_FILTER_INVALID_TIME_RANGE
    );

    assert(
        catalog_filter_radius(
            &source,
            100.0,
            0.0,
            50.0,
            &result
        ) ==
        CAMEFF_CATALOG_FILTER_INVALID_COORDINATES
    );

    assert(
        catalog_filter_radius(
            &source,
            0.0,
            0.0,
            -1.0,
            &result
        ) ==
        CAMEFF_CATALOG_FILTER_INVALID_RADIUS
    );
}

int main(void)
{
    test_catalog_initialization();
    test_valid_catalog_loading();
    test_invalid_catalog_rejection();
    test_missing_catalog_rejection();

    test_time_range_filtering();
    test_cutoff_prevents_future_leakage();
    test_surface_distance();
    test_radius_filtering();
    test_combined_hindcast_filter();
    test_invalid_filter_arguments();

    return 0;
}