#include "earthquake_catalog.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAMEFF_CATALOG_LINE_LENGTH 1024
#define CAMEFF_EXPECTED_HEADER \
    "time,latitude,longitude,depth,mag,region"

static void trim_line_ending(char *text)
{
    size_t length;

    if (text == NULL)
    {
        return;
    }

    length = strlen(text);

    while (length > 0 &&
           (text[length - 1] == '\n' ||
            text[length - 1] == '\r'))
    {
        text[length - 1] = '\0';
        length--;
    }
}

static char *trim_whitespace(char *text)
{
    char *end;

    if (text == NULL)
    {
        return NULL;
    }

    while (isspace((unsigned char)*text))
    {
        text++;
    }

    if (*text == '\0')
    {
        return text;
    }

    end = text + strlen(text) - 1;

    while (end > text &&
           isspace((unsigned char)*end))
    {
        *end = '\0';
        end--;
    }

    return text;
}

static char *remove_optional_quotes(
    char *text
)
{
    size_t length;

    if (text == NULL)
    {
        return NULL;
    }

    length = strlen(text);

    if (length >= 2U &&
        text[0] == '"' &&
        text[length - 1U] == '"')
    {
        text[length - 1U] = '\0';
        text++;
    }

    return text;
}

static int parse_double_field(
    const char *text,
    double *value
)
{
    char *end;
    double parsed;

    if (text == NULL || value == NULL)
    {
        return 0;
    }

    errno = 0;
    end = NULL;

    parsed = strtod(text, &end);

    if (errno != 0 || end == text)
    {
        return 0;
    }

    while (isspace((unsigned char)*end))
    {
        end++;
    }

    if (*end != '\0')
    {
        return 0;
    }

    *value = parsed;

    return 1;
}

/*
 * Converts a civil UTC date to Unix epoch days.
 *
 * Based on the Gregorian civil-calendar conversion algorithm.
 * This avoids relying on the non-standard timegm() function.
 */
static long long days_from_civil(
    int year,
    unsigned month,
    unsigned day
)
{
    int era;
    unsigned year_of_era;
    unsigned day_of_year;
    unsigned day_of_era;

    year -= month <= 2;

    era = year >= 0
        ? year / 400
        : (year - 399) / 400;

    year_of_era =
        (unsigned)(year - era * 400);

    day_of_year =
        (153U * (month + (month > 2 ? -3 : 9)) + 2U) /
            5U +
        day -
        1U;

    day_of_era =
        year_of_era * 365U +
        year_of_era / 4U -
        year_of_era / 100U +
        day_of_year;

    return
        (long long)era * 146097LL +
        (long long)day_of_era -
        719468LL;
}

static int parse_utc_timestamp(
    const char *text,
    time_t *timestamp
)
{
    int year;
    unsigned month;
    unsigned day;
    unsigned hour;
    unsigned minute;
    unsigned second;

    int consumed;
    const char *suffix;

    long long days;
    long long seconds;

    if (text == NULL || timestamp == NULL)
    {
        return 0;
    }

    consumed = 0;

    if (sscanf(
            text,
            "%d-%u-%uT%u:%u:%u%n",
            &year,
            &month,
            &day,
            &hour,
            &minute,
            &second,
            &consumed
        ) != 6)
    {
        return 0;
    }

    suffix = text + consumed;

    /*
     * Accept either:
     *
     *     YYYY-MM-DDTHH:MM:SSZ
     *
     * or:
     *
     *     YYYY-MM-DDTHH:MM:SS.sssZ
     */
    if (*suffix == '.')
    {
        suffix++;

        if (!isdigit((unsigned char)*suffix))
        {
            return 0;
        }

        while (isdigit((unsigned char)*suffix))
        {
            suffix++;
        }
    }

    if (suffix[0] != 'Z' ||
        suffix[1] != '\0')
    {
        return 0;
    }

    if (month < 1U || month > 12U ||
        day < 1U || day > 31U ||
        hour > 23U ||
        minute > 59U ||
        second > 60U)
    {
        return 0;
    }

    days = days_from_civil(
        year,
        month,
        day
    );

    seconds =
        days * 86400LL +
        (long long)hour * 3600LL +
        (long long)minute * 60LL +
        (long long)second;

    *timestamp = (time_t)seconds;

    return 1;
}

static int parse_catalog_row(
    char *line,
    EarthquakeEvent *event
)
{
    char *fields[6];
    char *cursor;
    char *separator;
    size_t index;

    if (line == NULL || event == NULL)
    {
        return 0;
    }

    /*
     * Split only the first five commas.
     *
     * The sixth field, region, may itself contain commas.
     */
    cursor = line;

    for (index = 0; index < 5; index++)
    {
        fields[index] = cursor;

        separator = strchr(cursor, ',');

        if (separator == NULL)
        {
            return 0;
        }

        *separator = '\0';
        cursor = separator + 1;
    }

    fields[5] = cursor;

    for (index = 0U; index < 6U; index++)
    {
        fields[index] =
            trim_whitespace(fields[index]);

        fields[index] =
            remove_optional_quotes(fields[index]);

        if (fields[index] == NULL ||
            fields[index][0] == '\0')
        {
            return 0;
        }
    }

    initialize_earthquake_event(event);

    if (!parse_utc_timestamp(
            fields[0],
            &event->timestamp))
    {
        return 0;
    }

    if (!parse_double_field(
            fields[1],
            &event->latitude))
    {
        return 0;
    }

    if (!parse_double_field(
            fields[2],
            &event->longitude))
    {
        return 0;
    }

    if (!parse_double_field(
            fields[3],
            &event->depth_km))
    {
        return 0;
    }

    if (!parse_double_field(
            fields[4],
            &event->magnitude))
    {
        return 0;
    }

    if (event->latitude < -90.0 ||
        event->latitude > 90.0)
    {
        return 0;
    }

    if (event->longitude < -180.0 ||
        event->longitude > 180.0)
    {
        return 0;
    }

    if (event->depth_km < 0.0)
    {
        return 0;
    }

    if (event->magnitude < -2.0 ||
        event->magnitude > 10.5)
    {
        return 0;
    }

    if (strlen(fields[5]) >= sizeof(event->region))
    {
        return 0;
    }

    memcpy(
        event->region,
        fields[5],
        strlen(fields[5]) + 1
    );

    return 1;
}

void catalog_initialize(
    EarthquakeCatalog *catalog
)
{
    if (catalog == NULL)
    {
        return;
    }

    catalog->count = 0;
}

int catalog_load_csv(
    const char *filename,
    EarthquakeCatalog *catalog
)
{
    FILE *file;
    char line[CAMEFF_CATALOG_LINE_LENGTH];

    if (filename == NULL || catalog == NULL)
    {
        return CAMEFF_CATALOG_INVALID_ARGUMENT;
    }

    catalog_initialize(catalog);

    file = fopen(filename, "r");

    if (file == NULL)
    {
        return CAMEFF_CATALOG_OPEN_FAILED;
    }

    if (fgets(line, sizeof(line), file) == NULL)
    {
        fclose(file);
        return CAMEFF_CATALOG_EMPTY_FILE;
    }

    trim_line_ending(line);

    if (strcmp(line, CAMEFF_EXPECTED_HEADER) != 0)
    {
        fclose(file);
        return CAMEFF_CATALOG_INVALID_HEADER;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        EarthquakeEvent event;
        char *trimmed;

        trim_line_ending(line);
        trimmed = trim_whitespace(line);

        /*
         * Empty lines are harmless and are skipped.
         */
        if (trimmed[0] == '\0')
        {
            continue;
        }

        if (catalog->count >=
            CAMEFF_CATALOG_MAX_EVENTS)
        {
            fclose(file);
            return CAMEFF_CATALOG_CAPACITY_EXCEEDED;
        }

        if (!parse_catalog_row(trimmed, &event))
        {
            fclose(file);
            catalog_initialize(catalog);

            return CAMEFF_CATALOG_INVALID_ROW;
        }

        catalog->events[catalog->count] = event;
        catalog->count++;
    }

    if (ferror(file))
    {
        fclose(file);
        catalog_initialize(catalog);

        return CAMEFF_CATALOG_INVALID_ROW;
    }

    fclose(file);

    return CAMEFF_CATALOG_OK;
}