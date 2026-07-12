#include "cameff/cameff.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "check failed: %s at line %d\n", #condition, __LINE__); return 1; } } while (0)

static cameff_event_t make_event(int64_t timestamp, double latitude, double longitude,
                                 double depth, double magnitude) {
    cameff_event_t event;
    (void)memset(&event, 0, sizeof(event));
    event.epoch_seconds = timestamp;
    event.latitude_deg = latitude;
    event.longitude_deg = longitude;
    event.depth_km = depth;
    event.magnitude = magnitude;
    return event;
}

int main(void) {
    cameff_catalog_t catalog;
    cameff_config_t config;
    cameff_analysis_window_t window;
    cameff_signal_frame_t frame;
    cameff_assessment_t assessment;
    int64_t cutoff = 0;
    size_t i;

    CHECK(fabs(cameff_haversine_km(0.0, 0.0, 0.0, 1.0) - 111.195) < 0.2);
    CHECK(cameff_parse_iso8601_utc("2026-01-31T00:00:00Z", &cutoff) == CAMEFF_STATUS_OK);
    cameff_catalog_init(&catalog);
    cameff_config_default(&config);
    for (i = 0U; i < 12U; i++) {
        cameff_event_t event = make_event(cutoff - (int64_t)(11U - i) * 3600,
                                          10.0 + 0.01 * (double)i, 123.0,
                                          40.0 - (double)i, 4.0 + 0.08 * (double)i);
        CHECK(cameff_catalog_append(&catalog, &event) == CAMEFF_STATUS_OK);
    }
    window = (cameff_analysis_window_t){10.0, 123.0, 100.0, 7U, cutoff};
    CHECK(cameff_extract_signal_frame(&catalog, &window, &config, &frame) == CAMEFF_STATUS_OK);
    CHECK(frame.selected_event_count == 12U);
    CHECK(frame.signals[CAMEFF_SIGNAL_TEMPORAL_ACCELERATION].value > 0.5);
    assessment = cameff_assess(&frame, &config);
    CHECK(assessment.available_signal_count >= 7U);
    CHECK(assessment.level != CAMEFF_DECISION_INSUFFICIENT_DATA);
    cameff_catalog_free(&catalog);
    puts("all tests passed");
    return 0;
}
