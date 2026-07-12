#ifndef CAMEFF_REGION_H
#define CAMEFF_REGION_H

#define CAMEFF_MAX_REGION_ID 64U

typedef enum {
    CAMEFF_TECTONIC_UNKNOWN = 0,
    CAMEFF_TECTONIC_SUBDUCTION = 1,
    CAMEFF_TECTONIC_CRUSTAL = 2,
    CAMEFF_TECTONIC_TRANSFORM = 3,
    CAMEFF_TECTONIC_VOLCANIC = 4,
    CAMEFF_TECTONIC_MIXED = 5
} cameff_tectonic_setting_t;

typedef struct {
    char region_id[CAMEFF_MAX_REGION_ID];
    cameff_tectonic_setting_t tectonic_setting;

    double subduction_affinity;
    double crustal_fault_affinity;
    double transform_affinity;
    double volcanic_affinity;

    double fault_map_confidence;
    double catalog_completeness;
    double station_coverage;
    double regional_prior_confidence;
} cameff_region_profile_t;

#endif
