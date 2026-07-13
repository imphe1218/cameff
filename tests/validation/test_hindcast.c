#include "../../src/data/earthquake_catalog.h"
#include "../../src/validation/hindcast.h"

#include <assert.h>
#include <stddef.h>

#ifndef CAMEFF_TEST_SOURCE_DIR
#error "CAMEFF_TEST_SOURCE_DIR is not defined"
#endif

static EarthquakeCatalog load_test_catalog(void)
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

    return catalog;
}

static EarthquakeEvent target_event(
    const EarthquakeCatalog *catalog
)
{
    assert(catalog != NULL);
    assert(catalog->count == 3);

    return catalog->events[2];
}

static void test_experiment_initialization(void)
{
    HindcastExperiment experiment;

    hindcast_experiment_initialize(
        &experiment
    );

    assert(experiment.source_catalog == NULL);
    assert(experiment.observation_start == 0);
    assert(experiment.cutoff_time == 0);
    assert(experiment.analysis_radius_km == 0.0);
}

static void test_result_initialization(void)
{
    HindcastResult result;

    hindcast_result_initialize(&result);

    assert(
        result.status ==
        CAMEFF_HINDCAST_NOT_EVALUATED
    );

    assert(result.evidence_event_count == 0);
    assert(result.hazard_score == 0.0);
    assert(result.confidence == 0.0);
    assert(result.dominant_expert[0] == '\0');
}

static void test_valid_experiment_contract(void)
{
    EarthquakeCatalog catalog;
    HindcastExperiment experiment;
    int status;

    catalog = load_test_catalog();

    hindcast_experiment_initialize(
        &experiment
    );

    experiment.target =
        target_event(&catalog);

    experiment.source_catalog = &catalog;
    experiment.observation_start =
        (time_t)1299600000;

    experiment.cutoff_time =
        experiment.target.timestamp;

    experiment.analysis_radius_km = 100.0;

    status =
        hindcast_experiment_validate(
            &experiment
        );

    assert(status == CAMEFF_HINDCAST_OK);
}

static void test_future_cutoff_rejected(void)
{
    EarthquakeCatalog catalog;
    HindcastExperiment experiment;

    catalog = load_test_catalog();

    hindcast_experiment_initialize(
        &experiment
    );

    experiment.target =
        target_event(&catalog);

    experiment.source_catalog = &catalog;
    experiment.observation_start =
        (time_t)1299600000;

    experiment.cutoff_time =
        experiment.target.timestamp + 1;

    experiment.analysis_radius_km = 100.0;

    assert(
        hindcast_experiment_validate(
            &experiment
        ) ==
        CAMEFF_HINDCAST_INVALID_TIME_WINDOW
    );
}

static void test_hindcast_constructs_evidence_window(void)
{
    EarthquakeCatalog catalog;
    HindcastExperiment experiment;
    HindcastResult result;
    int status;

    catalog = load_test_catalog();

    hindcast_experiment_initialize(
        &experiment
    );

    experiment.target =
        target_event(&catalog);

    experiment.source_catalog = &catalog;
    experiment.observation_start =
        (time_t)1299600000;

    experiment.cutoff_time =
        experiment.target.timestamp;

    experiment.analysis_radius_km = 100.0;

    status = run_hindcast(
        &experiment,
        &result
    );

    assert(
        status ==
        CAMEFF_HINDCAST_NOT_EVALUATED
    );

    assert(
        result.status ==
        CAMEFF_HINDCAST_NOT_EVALUATED
    );

    assert(result.evidence_event_count == 2);

    assert(
        result.cutoff_time ==
        experiment.target.timestamp
    );

    assert(result.hazard_score == 0.0);
    assert(result.confidence == 0.0);
    assert(result.dominant_expert[0] == '\0');
}

static void test_empty_evidence_window(void)
{
    EarthquakeCatalog catalog;
    HindcastExperiment experiment;
    HindcastResult result;
    int status;

    catalog = load_test_catalog();

    hindcast_experiment_initialize(
        &experiment
    );

    experiment.target =
        target_event(&catalog);

    experiment.source_catalog = &catalog;

    /*
     * This very narrow interval contains no prior events.
     */
    experiment.observation_start =
        experiment.target.timestamp - 10;

    experiment.cutoff_time =
        experiment.target.timestamp;

    experiment.analysis_radius_km = 100.0;

    status = run_hindcast(
        &experiment,
        &result
    );

    assert(
        status ==
        CAMEFF_HINDCAST_NO_EVIDENCE
    );

    assert(result.evidence_event_count == 0);
}

static void test_invalid_radius_rejected(void)
{
    EarthquakeCatalog catalog;
    HindcastExperiment experiment;

    catalog = load_test_catalog();

    hindcast_experiment_initialize(
        &experiment
    );

    experiment.target =
        target_event(&catalog);

    experiment.source_catalog = &catalog;
    experiment.observation_start =
        (time_t)1299600000;

    experiment.cutoff_time =
        experiment.target.timestamp;

    experiment.analysis_radius_km = 0.0;

    assert(
        hindcast_experiment_validate(
            &experiment
        ) ==
        CAMEFF_HINDCAST_INVALID_RADIUS
    );
}

int main(void)
{
    test_experiment_initialization();
    test_result_initialization();
    test_valid_experiment_contract();
    test_future_cutoff_rejected();
    test_hindcast_constructs_evidence_window();
    test_empty_evidence_window();
    test_invalid_radius_rejected();

    return 0;
}