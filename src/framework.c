#include "cameff/framework.h"

#include <math.h>

cameff_assessment_t cameff_assess(const cameff_signal_frame_t *frame,
                                  const cameff_config_t *config) {
    cameff_assessment_t result = {0.0, 0.0, CAMEFF_DECISION_INSUFFICIENT_DATA, 0U, 0U};
    double weighted_sum = 0.0;
    double used_weight = 0.0;
    unsigned i;

    if ((frame == NULL) || (config == NULL)) return result;
    for (i = 0U; i < CAMEFF_SIGNAL_COUNT; i++) {
        const cameff_signal_t *signal = &frame->signals[i];
        if (signal->available != 0) {
            const double effective_weight = config->weights[i] * signal->confidence;
            weighted_sum += effective_weight * signal->value;
            used_weight += effective_weight;
            result.available_signal_count++;
            if (signal->value < 0.30) result.contradictory_signal_count++;
        }
    }
    result.evidence_score = used_weight > 0.0 ? weighted_sum / used_weight : 0.0;
    result.confidence = fmin(1.0, fmax(0.0, frame->data_confidence));
    if ((frame->selected_event_count < config->minimum_events) ||
        (result.confidence < config->minimum_data_confidence)) {
        result.level = CAMEFF_DECISION_INSUFFICIENT_DATA;
    } else if (result.evidence_score >= config->elevated_threshold) {
        result.level = CAMEFF_DECISION_ELEVATED;
    } else if (result.evidence_score >= config->watch_threshold) {
        result.level = CAMEFF_DECISION_WATCH;
    } else {
        result.level = CAMEFF_DECISION_BACKGROUND;
    }
    return result;
}

const char *cameff_decision_level_name(cameff_decision_level_t level) {
    switch (level) {
        case CAMEFF_DECISION_BACKGROUND: return "BACKGROUND";
        case CAMEFF_DECISION_WATCH: return "WATCH";
        case CAMEFF_DECISION_ELEVATED: return "ELEVATED";
        case CAMEFF_DECISION_INSUFFICIENT_DATA:
        default: return "INSUFFICIENT_DATA";
    }
}
