#include "../src/data/earthquake_catalog.h"
#include "../src/validation/cameff_pipeline_adapter.h"
#include "../src/validation/hindcast.h"
#include "cameff/signals.h"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_unsigned_value(
    const char *text,
    unsigned *value
)
{
    char *end;
    unsigned long parsed;

    if (text == NULL || value == NULL)
    {
        return 0;
    }

    errno = 0;
    end = NULL;

    parsed = strtoul(text, &end, 10);

    if (errno != 0 ||
        end == text ||
        *end != '\0')
    {
        return 0;
    }

    *value = (unsigned)parsed;

    return 1;
}

static int parse_int64_value(
    const char *text,
    int64_t *value
)
{
    char *end;
    intmax_t parsed;

    if (text == NULL || value == NULL)
    {
        return 0;
    }

    errno = 0;
    end = NULL;

    parsed = strtoimax(text, &end, 10);

    if (errno != 0 ||
        end == text ||
        *end != '\0')
    {
        return 0;
    }

    *value = (int64_t)parsed;

    return 1;
}

static int parse_double_value(
    const char *text,
    double *value
)
{
    char *end;
    double parsed;

    if (text == NULL || value == NULL)
    {
        return 0;
    }

    errno = 0;
    end = NULL;

    parsed = strtod(text, &end);

    if (errno != 0 ||
        end == text ||
        *end != '\0')
    {
        return 0;
    }

    *value = parsed;

    return 1;
}

static void initialize_subduction_region(
    cameff_region_profile_t *region
)
{
    if (region == NULL)
    {
        return;
    }

    (void)memset(
        region,
        0,
        sizeof(*region)
    );

    (void)snprintf(
        region->region_id,
        sizeof(region->region_id),
        "%s",
        "japan-trench"
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

static const char *hindcast_status_name(
    int status
)
{
    switch (status)
    {
        case CAMEFF_HINDCAST_OK:
            return "ok";

        case CAMEFF_HINDCAST_INVALID_ARGUMENT:
            return "invalid-argument";

        case CAMEFF_HINDCAST_INVALID_TARGET:
            return "invalid-target";

        case CAMEFF_HINDCAST_INVALID_TIME_WINDOW:
            return "invalid-time-window";

        case CAMEFF_HINDCAST_INVALID_RADIUS:
            return "invalid-radius";

        case CAMEFF_HINDCAST_FILTER_FAILED:
            return "filter-failed";

        case CAMEFF_HINDCAST_NO_EVIDENCE:
            return "no-evidence";

        case CAMEFF_HINDCAST_EVALUATION_FAILED:
            return "evaluation-failed";

        case CAMEFF_HINDCAST_NOT_EVALUATED:
            return "not-evaluated";

        default:
            return "unknown";
    }
}

static void print_usage(
    const char *program_name
)
{
    (void)fprintf(
        stderr,
        "Usage:\n"
        "  %s <catalog.csv> "
        "<target-epoch> <target-latitude> "
        "<target-longitude> <target-depth-km> "
        "<target-magnitude> <target-region> "
        "<observation-start-epoch> <cutoff-epoch> "
        "<center-latitude> <center-longitude> "
        "<radius-km> <lookback-days> <minimum-events>\n",
        program_name
    );
}

int main(
    int argc,
    char **argv
)
{
    EarthquakeCatalog catalog;
    HindcastExperiment experiment;
    HindcastResult result;
    CameffPipelineAdapterContext adapter_context;

    unsigned lookback_days;
    unsigned minimum_events;

    int64_t target_epoch;
    int64_t observation_start_epoch;
    int64_t cutoff_epoch;

    double target_latitude;
    double target_longitude;
    double target_depth_km;
    double target_magnitude;

    double center_latitude;
    double center_longitude;
    double radius_km;

    const char *target_region;

    int catalog_status;
    int hindcast_status;

    if (argc != 15)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    target_region = argv[7];

    if (!parse_int64_value(
            argv[2],
            &target_epoch) ||
        !parse_double_value(
            argv[3],
            &target_latitude) ||
        !parse_double_value(
            argv[4],
            &target_longitude) ||
        !parse_double_value(
            argv[5],
            &target_depth_km) ||
        !parse_double_value(
            argv[6],
            &target_magnitude) ||
        !parse_int64_value(
            argv[8],
            &observation_start_epoch) ||
        !parse_int64_value(
            argv[9],
            &cutoff_epoch) ||
        !parse_double_value(
            argv[10],
            &center_latitude) ||
        !parse_double_value(
            argv[11],
            &center_longitude) ||
        !parse_double_value(
            argv[12],
            &radius_km) ||
        !parse_unsigned_value(
            argv[13],
            &lookback_days) ||
        !parse_unsigned_value(
            argv[14],
            &minimum_events))
    {
        (void)fprintf(
            stderr,
            "Invalid command-line argument.\n"
        );

        print_usage(argv[0]);

        return EXIT_FAILURE;
    }

if (strlen(target_region) >=
    sizeof(experiment.target.region))
{
    (void)fprintf(
        stderr,
        "Target region exceeds the maximum length.\n"
    );

    return EXIT_FAILURE;
}

    catalog_status =
        catalog_load_csv(
            argv[1],
            &catalog
        );

    if (catalog_status != CAMEFF_CATALOG_OK)
    {
        (void)fprintf(
            stderr,
            "Failed to load catalog: status=%d\n",
            catalog_status
        );

        return EXIT_FAILURE;
    }

    hindcast_experiment_initialize(
        &experiment
    );

    initialize_earthquake_event(
        &experiment.target
    );

    experiment.target.timestamp =
        (time_t)target_epoch;

    experiment.target.latitude =
        target_latitude;

    experiment.target.longitude =
        target_longitude;

    experiment.target.depth_km =
        target_depth_km;

    experiment.target.magnitude =
        target_magnitude;

    (void)snprintf(
        experiment.target.region,
        sizeof(experiment.target.region),
        "%s",
        target_region
    );

    experiment.source_catalog = &catalog;

    experiment.observation_start =
        (time_t)observation_start_epoch;

    experiment.cutoff_time =
        (time_t)cutoff_epoch;

    experiment.analysis_radius_km =
        radius_km;

    cameff_pipeline_adapter_context_initialize(
        &adapter_context
    );

    adapter_context.analysis_latitude =
        center_latitude;

    adapter_context.analysis_longitude =
        center_longitude;

    adapter_context.analysis_radius_km =
        radius_km;

    adapter_context.lookback_days =
        lookback_days;

    adapter_context.cutoff_epoch_seconds =
        cutoff_epoch;

    adapter_context.config.minimum_events =
        minimum_events;

    initialize_subduction_region(
        &adapter_context.region
    );

    experiment.evaluator =
        cameff_pipeline_evaluator;

    experiment.evaluator_context =
        &adapter_context;

    hindcast_status =
        run_hindcast(
            &experiment,
            &result
        );

    (void)printf(
        "experiment_status=%s\n",
        hindcast_status_name(hindcast_status)
    );

    (void)printf(
        "experiment_status_code=%d\n",
        hindcast_status
    );

    (void)printf(
        "target_region=%s\n",
        experiment.target.region
    );

    (void)printf(
        "target_timestamp=%" PRId64 "\n",
        (int64_t)experiment.target.timestamp
    );

    (void)printf(
        "target_latitude=%.6f\n",
        experiment.target.latitude
    );

    (void)printf(
        "target_longitude=%.6f\n",
        experiment.target.longitude
    );

    (void)printf(
        "target_depth_km=%.6f\n",
        experiment.target.depth_km
    );

    (void)printf(
        "target_magnitude=%.6f\n",
        experiment.target.magnitude
    );

    (void)printf(
        "observation_start=%" PRId64 "\n",
        observation_start_epoch
    );

    (void)printf(
        "cutoff=%" PRId64 "\n",
        cutoff_epoch
    );

    (void)printf(
        "analysis_center_latitude=%.6f\n",
        center_latitude
    );

    (void)printf(
        "analysis_center_longitude=%.6f\n",
        center_longitude
    );

    (void)printf(
        "analysis_radius_km=%.6f\n",
        radius_km
    );

    (void)printf(
        "lookback_days=%u\n",
        lookback_days
    );

    (void)printf(
        "minimum_events=%u\n",
        minimum_events
    );

    (void)printf(
        "evidence_event_count=%zu\n",
        result.evidence_event_count
    );

    if (hindcast_status == CAMEFF_HINDCAST_OK)
    {
        (void)printf(
            "hazard_evidence=%.6f\n",
            result.hazard_score
        );

        (void)printf(
            "fused_confidence=%.6f\n",
            result.confidence
        );

        (void)printf(
            "dominant_pattern=%s\n",
            result.dominant_pattern
        );

        (void)printf(
            "dominant_expert=%s\n",
            result.dominant_expert
        );

        {
            size_t signal_index;

            for (signal_index = 0U;
                signal_index < result.signal_count;
                signal_index++)
            {
                const cameff_signal_t *signal;
                const char *signal_name;

                signal =
                    &result.signals[signal_index];

                signal_name =
                    cameff_signal_name(
                        (cameff_signal_id_t)signal_index
                    );

                (void)printf(
                    "signal.%s.value=%.6f\n",
                    signal_name,
                    signal->value
                );

                (void)printf(
                    "signal.%s.confidence=%.6f\n",
                    signal_name,
                    signal->confidence
                );

                (void)printf(
                    "signal.%s.supporting_events=%zu\n",
                    signal_name,
                    signal->supporting_events
                );

                (void)printf(
                    "signal.%s.available=%d\n",
                    signal_name,
                    signal->available
                );
            }
        }
    }

    return hindcast_status == CAMEFF_HINDCAST_OK
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}