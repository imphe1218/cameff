#include "../../src/data/earthquake_catalog.h"
#include "../../src/validation/cameff_pipeline_adapter.h"
#include "../../src/validation/hindcast.h"

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

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
    assert(experiment.evaluator == NULL);
    assert(experiment.evaluator_context == NULL);
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
    assert(result.dominant_pattern[0] == '\0');
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
    assert(result.dominant_pattern[0] == '\0');
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

static int mock_successful_evaluator(
    const EarthquakeCatalog *evidence_catalog,
    const EarthquakeEvent *target_context,
    CameffEvaluationOutput *output,
    void *user_context
)
{
    (void)target_context;
    (void)user_context;

    assert(evidence_catalog != NULL);
    assert(evidence_catalog->count == 2);
    assert(output != NULL);

    output->hazard_score = 0.75;
    output->confidence = 0.80;

    strcpy(
        output->dominant_pattern,
        "subduction-preparation"
    );

    strcpy(
        output->dominant_expert,
        "subduction"
    );

    return CAMEFF_EVALUATOR_OK;
}

static int mock_invalid_evaluator(
    const EarthquakeCatalog *evidence_catalog,
    const EarthquakeEvent *target_context,
    CameffEvaluationOutput *output,
    void *user_context
)
{
    (void)evidence_catalog;
    (void)target_context;
    (void)user_context;

    output->hazard_score = 1.5;
    output->confidence = 0.8;

    strcpy(
        output->dominant_pattern,
        "invalid"
    );

    strcpy(
        output->dominant_expert,
        "invalid"
    );

    return CAMEFF_EVALUATOR_OK;
}

static void test_hindcast_invokes_evaluator(void)
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

    experiment.evaluator =
        mock_successful_evaluator;

    status = run_hindcast(
        &experiment,
        &result
    );

    assert(status == CAMEFF_HINDCAST_OK);
    assert(result.status == CAMEFF_HINDCAST_OK);
    assert(result.evidence_event_count == 2);
    assert(result.hazard_score == 0.75);
    assert(result.confidence == 0.80);

    assert(
        strcmp(
            result.dominant_pattern,
            "subduction-preparation"
        ) == 0
    );

    assert(
        strcmp(
            result.dominant_expert,
            "subduction"
        ) == 0
    );
}

static void test_invalid_evaluator_output_rejected(void)
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

    experiment.evaluator =
        mock_invalid_evaluator;

    status = run_hindcast(
        &experiment,
        &result
    );

    assert(
        status ==
        CAMEFF_HINDCAST_EVALUATION_FAILED
    );

    assert(
        result.status ==
        CAMEFF_HINDCAST_EVALUATION_FAILED
    );
}

static void initialize_test_subduction_region(
    cameff_region_profile_t *region
)
{
    assert(region != NULL);

    (void)memset(
        region,
        0,
        sizeof(*region)
    );

    (void)snprintf(
        region->region_id,
        sizeof(region->region_id),
        "%s",
        "japan-trench-test"
    );

    region->tectonic_setting =
        CAMEFF_TECTONIC_SUBDUCTION;

    region->subduction_affinity = 1.0;
    region->crustal_fault_affinity = 0.10;
    region->transform_affinity = 0.05;
    region->volcanic_affinity = 0.05;

    region->fault_map_confidence = 0.90;
    region->catalog_completeness = 0.90;
    region->station_coverage = 0.90;
    region->regional_prior_confidence = 0.90;
}

static void test_real_pipeline_adapter_rejects_missing_cutoff(void)
{
    EarthquakeCatalog catalog;
    HindcastExperiment experiment;
    HindcastResult result;
    CameffPipelineAdapterContext adapter_context;
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

    cameff_pipeline_adapter_context_initialize(
        &adapter_context
    );

    adapter_context.analysis_latitude =
        experiment.target.latitude;

    adapter_context.analysis_longitude =
        experiment.target.longitude;

    adapter_context.analysis_radius_km =
        experiment.analysis_radius_km;

    adapter_context.lookback_days = 365U;

    /*
     * Deliberately leave:
     *
     * adapter_context.cutoff_epoch_seconds == 0
     *
     * The adapter must reject the invalid context.
     */
    initialize_test_subduction_region(
        &adapter_context.region
    );

    experiment.evaluator =
        cameff_pipeline_evaluator;

    experiment.evaluator_context =
        &adapter_context;

    status = run_hindcast(
        &experiment,
        &result
    );

    assert(
        status ==
        CAMEFF_HINDCAST_EVALUATION_FAILED
    );
}

static void test_real_pipeline_adapter(void)
{
    EarthquakeCatalog catalog;
    HindcastExperiment experiment;
    HindcastResult result;
    CameffPipelineAdapterContext adapter_context;
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

    cameff_pipeline_adapter_context_initialize(
        &adapter_context
    );

    adapter_context.analysis_latitude =
        experiment.target.latitude;

    adapter_context.analysis_longitude =
        experiment.target.longitude;

    adapter_context.analysis_radius_km =
        experiment.analysis_radius_km;

    adapter_context.lookback_days = 365U;

    /*
     * This is the required assignment.
     *
     * It is placed after experiment.cutoff_time has been
     * configured and before run_hindcast() is called.
     */
    adapter_context.cutoff_epoch_seconds =
        (int64_t)experiment.cutoff_time;

    initialize_test_subduction_region(
        &adapter_context.region
    );

    /*
     * The fixture contains only two pre-event earthquakes.
     * Lower the minimum for this small integration fixture.
     *
     * This is test configuration only, not operational
     * calibration.
     */
    adapter_context.config.minimum_events = 2U;

    experiment.evaluator =
        cameff_pipeline_evaluator;

    experiment.evaluator_context =
        &adapter_context;

    status = run_hindcast(
        &experiment,
        &result
    );

    assert(status == CAMEFF_HINDCAST_OK);
    assert(result.status == CAMEFF_HINDCAST_OK);
    assert(result.evidence_event_count == 2);

    assert(result.hazard_score >= 0.0);
    assert(result.hazard_score <= 1.0);

    assert(result.confidence >= 0.0);
    assert(result.confidence <= 1.0);

    assert(result.dominant_pattern[0] != '\0');
    assert(result.dominant_expert[0] != '\0');
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
    test_hindcast_invokes_evaluator();
    test_invalid_evaluator_output_rejected();
    test_real_pipeline_adapter_rejects_missing_cutoff();
    test_real_pipeline_adapter();

    return 0;
}