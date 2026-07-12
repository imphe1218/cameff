#include "cameff/config.h"
#include "cameff/signals.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cameff_config_default(cameff_config_t *config) {
    static const double default_weights[CAMEFF_SIGNAL_COUNT] = {
        0.17, 0.18, 0.15, 0.14, 0.10, 0.12, 0.08, 0.06
    };
    if (config == NULL) {
        return;
    }
    config->minimum_events = 8U;
    config->watch_threshold = 0.45;
    config->elevated_threshold = 0.68;
    config->minimum_data_confidence = 0.35;
    config->fault_map_confidence = 0.50;
    (void)memcpy(config->weights, default_weights, sizeof(default_weights));
}

static void trim(char *text) {
    char *start = text;
    char *end;
    while ((*start == ' ') || (*start == '\t')) {
        start++;
    }
    if (start != text) {
        (void)memmove(text, start, strlen(start) + 1U);
    }
    end = text + strlen(text);
    while ((end > text) && ((end[-1] == ' ') || (end[-1] == '\t') || (end[-1] == '\r') || (end[-1] == '\n'))) {
        end--;
    }
    *end = '\0';
}

static int parse_number(const char *text, double *value) {
    char *end = NULL;
    errno = 0;
    *value = strtod(text, &end);
    return (errno == 0) && (end != text) && (*end == '\0');
}

cameff_status_t cameff_config_load(const char *path, cameff_config_t *config,
                                   char *error_message, size_t error_message_size) {
    FILE *file;
    char line[256];
    size_t line_number = 0U;

    if ((path == NULL) || (config == NULL)) {
        return CAMEFF_STATUS_INVALID_ARGUMENT;
    }
    file = fopen(path, "r");
    if (file == NULL) {
        (void)snprintf(error_message, error_message_size, "cannot open configuration file");
        return CAMEFF_STATUS_IO_ERROR;
    }
    while (fgets(line, (int)sizeof(line), file) != NULL) {
        char *separator;
        char *key;
        char *value_text;
        double value;
        line_number++;
        trim(line);
        if ((line[0] == '\0') || (line[0] == '#')) {
            continue;
        }
        separator = strchr(line, '=');
        if (separator == NULL) {
            (void)snprintf(error_message, error_message_size, "line %zu: expected key=value", line_number);
            (void)fclose(file);
            return CAMEFF_STATUS_PARSE_ERROR;
        }
        *separator = '\0';
        key = line;
        value_text = separator + 1;
        trim(key);
        trim(value_text);
        if (!parse_number(value_text, &value)) {
            (void)snprintf(error_message, error_message_size, "line %zu: invalid number", line_number);
            (void)fclose(file);
            return CAMEFF_STATUS_PARSE_ERROR;
        }
        if (strcmp(key, "minimum_events") == 0) config->minimum_events = (size_t)value;
        else if (strcmp(key, "watch_threshold") == 0) config->watch_threshold = value;
        else if (strcmp(key, "elevated_threshold") == 0) config->elevated_threshold = value;
        else if (strcmp(key, "minimum_data_confidence") == 0) config->minimum_data_confidence = value;
        else if (strcmp(key, "fault_map_confidence") == 0) config->fault_map_confidence = value;
        else if (strncmp(key, "weight.", 7U) == 0) {
            size_t i;
            int found = 0;
            for (i = 0U; i < CAMEFF_SIGNAL_COUNT; i++) {
                if (strcmp(key + 7, cameff_signal_name((cameff_signal_id_t)i)) == 0) {
                    config->weights[i] = value;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                (void)snprintf(error_message, error_message_size, "line %zu: unknown signal weight", line_number);
                (void)fclose(file);
                return CAMEFF_STATUS_PARSE_ERROR;
            }
        } else {
            (void)snprintf(error_message, error_message_size, "line %zu: unknown configuration key", line_number);
            (void)fclose(file);
            return CAMEFF_STATUS_PARSE_ERROR;
        }
    }
    (void)fclose(file);
    return CAMEFF_STATUS_OK;
}
