#ifndef CAMEFF_SIGNALS_H
#define CAMEFF_SIGNALS_H

#include "cameff/types.h"

const char *cameff_signal_name(cameff_signal_id_t signal_id);
cameff_status_t cameff_extract_signal_frame(const cameff_catalog_t *catalog,
                                            const cameff_analysis_window_t *window,
                                            const cameff_config_t *config,
                                            cameff_signal_frame_t *frame);

#endif
