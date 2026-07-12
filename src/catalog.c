#include "cameff/catalog.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void set_error(char *buffer, size_t size, const char *message) {
    if ((buffer != NULL) && (size > 0U)) {
        (void)snprintf(buffer, size, "%s", message);
    }
}

void cameff_catalog_init(cameff_catalog_t *catalog) {
    if (catalog != NULL) {
        catalog->items = NULL;
        catalog->count = 0U;
        catalog->capacity = 0U;
    }
}

void cameff_catalog_free(cameff_catalog_t *catalog) {
    if (catalog != NULL) {
        free(catalog->items);
        cameff_catalog_init(catalog);
    }
}

cameff_status_t cameff_catalog_append(cameff_catalog_t *catalog, const cameff_event_t *event) {
    if ((catalog == NULL) || (event == NULL)) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }
    if (catalog->count == catalog->capacity) {
        const size_t next_capacity = (catalog->capacity == 0U) ? 64U : catalog->capacity * 2U;
        cameff_event_t *next_items = realloc(catalog->items, next_capacity * sizeof(*next_items));
        if (next_items == NULL) {
            return CAMEFF_STATUS_OUT_OF_MEMORY;
        }
        catalog->items = next_items;
        catalog->capacity = next_capacity;
    }
    catalog->items[catalog->count] = *event;
    catalog->count++;
    return CAMEFF_STATUS_OK;
}

cameff_status_t cameff_parse_iso8601_utc(const char *text, int64_t *epoch_seconds) {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    struct tm value;
    time_t timestamp;

    if ((text == NULL) || (epoch_seconds == NULL)) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }
    if (sscanf(text, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
        return CAMEFF_STATUS_PARSE_ERROR;
    }
    (void)memset(&value, 0, sizeof(value));
    value.tm_year = year - 1900;
    value.tm_mon = month - 1;
    value.tm_mday = day;
    value.tm_hour = hour;
    value.tm_min = minute;
    value.tm_sec = second;
#ifdef _WIN32
    timestamp = _mkgmtime(&value);
#else
    timestamp = timegm(&value);
#endif
    if (timestamp == (time_t)-1) {
        return CAMEFF_STATUS_PARSE_ERROR;
    }
    *epoch_seconds = (int64_t)timestamp;
    return CAMEFF_STATUS_OK;
}

static int parse_double(const char *text, double *value) {
    char *end = NULL;
    errno = 0;
    *value = strtod(text, &end);
    return (errno == 0) && (end != text) && (*end == '\0');
}

cameff_status_t cameff_catalog_load_csv(const char *path, cameff_catalog_t *catalog,
                                        char *error_message, size_t error_message_size) {
    FILE *file;
    char line[1024];
    size_t line_number = 0U;

    if ((path == NULL) || (catalog == NULL)) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }
    file = fopen(path, "r");
    if (file == NULL) {
        set_error(error_message, error_message_size, "cannot open catalog file");
        return CAMEFF_STATUS_IO_ERROR;
    }

    while (fgets(line, (int)sizeof(line), file) != NULL) {
        char *fields[7];
        size_t field_count = 0U;
        char *token;
        cameff_event_t event;
        cameff_status_t status;
        line_number++;

        line[strcspn(line, "\r\n")] = '\0';
        if ((line[0] == '\0') || (line[0] == '#')) {
            continue;
        }
        if ((line_number == 1U) && (strstr(line, "time") != NULL)) {
            continue;
        }
        token = strtok(line, ",");
        while ((token != NULL) && (field_count < 7U)) {
            fields[field_count++] = token;
            token = strtok(NULL, ",");
        }
        if (field_count != 7U) {
            (void)snprintf(error_message, error_message_size, "line %zu: expected 7 columns", line_number);
            (void)fclose(file);
            return CAMEFF_STATUS_PARSE_ERROR;
        }

        (void)memset(&event, 0, sizeof(event));
        (void)snprintf(event.id, sizeof(event.id), "%s", fields[0]);
        (void)snprintf(event.source, sizeof(event.source), "%s", fields[1]);
        status = cameff_parse_iso8601_utc(fields[2], &event.epoch_seconds);
        if ((status != CAMEFF_STATUS_OK) || !parse_double(fields[3], &event.latitude_deg) ||
            !parse_double(fields[4], &event.longitude_deg) || !parse_double(fields[5], &event.depth_km) ||
            !parse_double(fields[6], &event.magnitude)) {
            (void)snprintf(error_message, error_message_size, "line %zu: invalid event value", line_number);
            (void)fclose(file);
            return CAMEFF_STATUS_PARSE_ERROR;
        }
        status = cameff_catalog_append(catalog, &event);
        if (status != CAMEFF_STATUS_OK) {
            set_error(error_message, error_message_size, "out of memory while loading catalog");
            (void)fclose(file);
            return status;
        }
    }
    if (ferror(file) != 0) {
        set_error(error_message, error_message_size, "error while reading catalog file");
        (void)fclose(file);
        return CAMEFF_STATUS_IO_ERROR;
    }
    (void)fclose(file);
    return CAMEFF_STATUS_OK;
}
