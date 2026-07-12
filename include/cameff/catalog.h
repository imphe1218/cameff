#ifndef CAMEFF_CATALOG_H
#define CAMEFF_CATALOG_H

#include "cameff/types.h"

void cameff_catalog_init(cameff_catalog_t *catalog);
void cameff_catalog_free(cameff_catalog_t *catalog);
cameff_status_t cameff_catalog_append(cameff_catalog_t *catalog, const cameff_event_t *event);
cameff_status_t cameff_catalog_load_csv(const char *path, cameff_catalog_t *catalog,
                                        char *error_message, size_t error_message_size);
cameff_status_t cameff_parse_iso8601_utc(const char *text, int64_t *epoch_seconds);

#endif
