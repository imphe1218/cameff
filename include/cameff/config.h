#ifndef CAMEFF_CONFIG_H
#define CAMEFF_CONFIG_H

#include "cameff/types.h"

void cameff_config_default(cameff_config_t *config);
cameff_status_t cameff_config_load(const char *path, cameff_config_t *config,
                                   char *error_message, size_t error_message_size);

#endif
