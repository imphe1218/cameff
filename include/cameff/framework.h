#ifndef CAMEFF_FRAMEWORK_H
#define CAMEFF_FRAMEWORK_H

#include "cameff/types.h"

cameff_assessment_t cameff_assess(const cameff_signal_frame_t *frame,
                                  const cameff_config_t *config);
const char *cameff_decision_level_name(cameff_decision_level_t level);

#endif
