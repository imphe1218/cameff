#ifndef CAMEFF_TYPES_H
#define CAMEFF_TYPES_H

#include <stddef.h>
#include <stdint.h>

#define CAMEFF_SIGNAL_COUNT 8U
#define CAMEFF_MAX_EVENT_ID 64U
#define CAMEFF_MAX_SOURCE 32U

typedef enum {
    CAMEFF_STATUS_OK = 0,
    CAMEFF_STATUS_INVALID_ARGUMENT = 1,
    CAMEFF_STATUS_IO_ERROR = 2,
    CAMEFF_STATUS_PARSE_ERROR = 3,
    CAMEFF_STATUS_OUT_OF_MEMORY = 4,
    CAMEFF_STATUS_INSUFFICIENT_DATA = 5
} cameff_status_t;

typedef enum {
    CAMEFF_SIGNAL_ACTIVITY_RATE = 0,
    CAMEFF_SIGNAL_TEMPORAL_ACCELERATION = 1,
    CAMEFF_SIGNAL_SPATIAL_CONCENTRATION = 2,
    CAMEFF_SIGNAL_MAGNITUDE_TREND = 3,
    CAMEFF_SIGNAL_DEPTH_MIGRATION = 4,
    CAMEFF_SIGNAL_B_VALUE_ANOMALY = 5,
    CAMEFF_SIGNAL_CATALOG_COMPLETENESS = 6,
    CAMEFF_SIGNAL_FAULT_MAP_CONFIDENCE = 7
} cameff_signal_id_t;

typedef struct {
    char id[CAMEFF_MAX_EVENT_ID];
    char source[CAMEFF_MAX_SOURCE];
    int64_t epoch_seconds;
    double latitude_deg;
    double longitude_deg;
    double depth_km;
    double magnitude;
} cameff_event_t;

typedef struct {
    cameff_event_t *items;
    size_t count;
    size_t capacity;
} cameff_catalog_t;

typedef struct {
    double center_latitude_deg;
    double center_longitude_deg;
    double radius_km;
    unsigned lookback_days;
    int64_t cutoff_epoch_seconds;
} cameff_analysis_window_t;

typedef struct {
    double value;
    double confidence;
    size_t supporting_events;
    int available;
} cameff_signal_t;

typedef struct {
    cameff_signal_t signals[CAMEFF_SIGNAL_COUNT];
    size_t selected_event_count;
    double data_confidence;
} cameff_signal_frame_t;

typedef enum {
    CAMEFF_DECISION_INSUFFICIENT_DATA = 0,
    CAMEFF_DECISION_BACKGROUND = 1,
    CAMEFF_DECISION_WATCH = 2,
    CAMEFF_DECISION_ELEVATED = 3
} cameff_decision_level_t;

typedef struct {
    double evidence_score;
    double confidence;
    cameff_decision_level_t level;
    unsigned available_signal_count;
    unsigned contradictory_signal_count;
} cameff_assessment_t;

typedef struct {
    size_t minimum_events;
    double watch_threshold;
    double elevated_threshold;
    double minimum_data_confidence;
    double fault_map_confidence;
    double weights[CAMEFF_SIGNAL_COUNT];
} cameff_config_t;

#endif
