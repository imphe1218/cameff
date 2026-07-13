#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAMEFF_NORMALIZER_LINE_CAPACITY 8192U
#define CAMEFF_NORMALIZER_FIELD_CAPACITY 1024U
#define CAMEFF_NORMALIZER_MAX_COLUMNS 64U

typedef struct
{
    char values[CAMEFF_NORMALIZER_MAX_COLUMNS]
               [CAMEFF_NORMALIZER_FIELD_CAPACITY];

    size_t count;
} CsvRow;

typedef struct
{
    size_t time;
    size_t latitude;
    size_t longitude;
    size_t depth;
    size_t magnitude;
    size_t region;
} UsgsColumnMap;

static void remove_line_ending(
    char *line
)
{
    size_t length;

    if (line == NULL)
    {
        return;
    }

    length = strlen(line);

    while (length > 0U &&
           (line[length - 1U] == '\n' ||
            line[length - 1U] == '\r'))
    {
        line[length - 1U] = '\0';
        length--;
    }
}

static int append_character(
    char *field,
    size_t *length,
    char character
)
{
    if (field == NULL || length == NULL)
    {
        return 0;
    }

    if (*length + 1U >=
        CAMEFF_NORMALIZER_FIELD_CAPACITY)
    {
        return 0;
    }

    field[*length] = character;
    (*length)++;
    field[*length] = '\0';

    return 1;
}

static int parse_csv_row(
    const char *line,
    CsvRow *row
)
{
    size_t column;
    size_t field_length;
    size_t index;
    int inside_quotes;

    if (line == NULL || row == NULL)
    {
        return 0;
    }

    (void)memset(row, 0, sizeof(*row));

    column = 0U;
    field_length = 0U;
    index = 0U;
    inside_quotes = 0;

    while (line[index] != '\0')
    {
        char current;

        current = line[index];

        if (current == '"')
        {
            if (inside_quotes != 0 &&
                line[index + 1U] == '"')
            {
                if (!append_character(
                        row->values[column],
                        &field_length,
                        '"'))
                {
                    return 0;
                }

                index += 2U;
                continue;
            }

            inside_quotes = !inside_quotes;
            index++;
            continue;
        }

        if (current == ',' &&
            inside_quotes == 0)
        {
            column++;

            if (column >=
                CAMEFF_NORMALIZER_MAX_COLUMNS)
            {
                return 0;
            }

            field_length = 0U;
            index++;
            continue;
        }

        if (!append_character(
                row->values[column],
                &field_length,
                current))
        {
            return 0;
        }

        index++;
    }

    if (inside_quotes != 0)
    {
        return 0;
    }

    row->count = column + 1U;

    return 1;
}

static int find_column(
    const CsvRow *header,
    const char *name,
    size_t *index
)
{
    size_t column;

    if (header == NULL ||
        name == NULL ||
        index == NULL)
    {
        return 0;
    }

    for (column = 0U;
         column < header->count;
         column++)
    {
        if (strcmp(
                header->values[column],
                name) == 0)
        {
            *index = column;
            return 1;
        }
    }

    return 0;
}

static int build_usgs_column_map(
    const CsvRow *header,
    UsgsColumnMap *map
)
{
    if (header == NULL || map == NULL)
    {
        return 0;
    }

    return
        find_column(header, "time", &map->time) &&
        find_column(header, "latitude", &map->latitude) &&
        find_column(header, "longitude", &map->longitude) &&
        find_column(header, "depth", &map->depth) &&
        find_column(header, "mag", &map->magnitude) &&
        find_column(header, "place", &map->region);
}

static int validate_required_fields(
    const CsvRow *row,
    const UsgsColumnMap *map
)
{
    size_t largest_index;

    if (row == NULL || map == NULL)
    {
        return 0;
    }

    largest_index = map->time;

    if (map->latitude > largest_index)
    {
        largest_index = map->latitude;
    }

    if (map->longitude > largest_index)
    {
        largest_index = map->longitude;
    }

    if (map->depth > largest_index)
    {
        largest_index = map->depth;
    }

    if (map->magnitude > largest_index)
    {
        largest_index = map->magnitude;
    }

    if (map->region > largest_index)
    {
        largest_index = map->region;
    }

    if (row->count <= largest_index)
    {
        return 0;
    }

    if (row->values[map->time][0] == '\0' ||
        row->values[map->latitude][0] == '\0' ||
        row->values[map->longitude][0] == '\0' ||
        row->values[map->depth][0] == '\0' ||
        row->values[map->magnitude][0] == '\0' ||
        row->values[map->region][0] == '\0')
    {
        return 0;
    }

    return 1;
}

static void write_csv_field(
    FILE *output,
    const char *value
)
{
    const char *cursor;
    int needs_quotes;

    needs_quotes = 0;

    for (cursor = value;
         *cursor != '\0';
         cursor++)
    {
        if (*cursor == ',' ||
            *cursor == '"' ||
            *cursor == '\n' ||
            *cursor == '\r')
        {
            needs_quotes = 1;
            break;
        }
    }

    if (needs_quotes == 0)
    {
        (void)fputs(value, output);
        return;
    }

    (void)fputc('"', output);

    for (cursor = value;
         *cursor != '\0';
         cursor++)
    {
        if (*cursor == '"')
        {
            (void)fputc('"', output);
        }

        (void)fputc(*cursor, output);
    }

    (void)fputc('"', output);
}

static void normalize_region(
    const char *source,
    char *destination,
    size_t destination_capacity
)
{
    size_t source_index;
    size_t destination_index;

    if (source == NULL ||
        destination == NULL ||
        destination_capacity == 0U)
    {
        return;
    }

    destination_index = 0U;

    for (source_index = 0U;
         source[source_index] != '\0' &&
         destination_index + 1U < destination_capacity;
         source_index++)
    {
        char character;

        character = source[source_index];

        if (character == ',' ||
            character == '\n' ||
            character == '\r')
        {
            character = ' ';
        }

        destination[destination_index] =
            character;

        destination_index++;
    }

    destination[destination_index] = '\0';
}

static int normalize_catalog(
    const char *input_path,
    const char *output_path
)
{
    FILE *input;
    FILE *output;

    char line[CAMEFF_NORMALIZER_LINE_CAPACITY];

    CsvRow header;
    CsvRow row;
    UsgsColumnMap map;

    size_t input_rows;
    size_t output_rows;
    size_t rejected_rows;

    input = fopen(input_path, "r");

    if (input == NULL)
    {
        (void)fprintf(
            stderr,
            "Unable to open input catalog: %s\n",
            input_path
        );

        return 0;
    }

    output = fopen(output_path, "w");

    if (output == NULL)
    {
        (void)fprintf(
            stderr,
            "Unable to open output catalog: %s\n",
            output_path
        );

        (void)fclose(input);
        return 0;
    }

    if (fgets(
            line,
            sizeof(line),
            input) == NULL)
    {
        (void)fprintf(
            stderr,
            "Input catalog is empty.\n"
        );

        (void)fclose(output);
        (void)fclose(input);

        return 0;
    }

    remove_line_ending(line);

    if (!parse_csv_row(line, &header))
    {
        (void)fprintf(
            stderr,
            "Unable to parse USGS CSV header.\n"
        );

        (void)fclose(output);
        (void)fclose(input);

        return 0;
    }

    if (!build_usgs_column_map(
            &header,
            &map))
    {
        (void)fprintf(
            stderr,
            "USGS CSV is missing one or more required columns.\n"
        );

        (void)fclose(output);
        (void)fclose(input);

        return 0;
    }

    (void)fputs(
        "time,latitude,longitude,depth,mag,region\n",
        output
    );

    input_rows = 0U;
    output_rows = 0U;
    rejected_rows = 0U;

    while (fgets(
            line,
            sizeof(line),
            input) != NULL)
    {
        remove_line_ending(line);

        if (line[0] == '\0')
        {
            continue;
        }

        input_rows++;

        if (!parse_csv_row(line, &row) ||
            !validate_required_fields(
                &row,
                &map))
        {
            rejected_rows++;
            continue;
        }

        write_csv_field(
            output,
            row.values[map.time]
        );

        (void)fputc(',', output);

        write_csv_field(
            output,
            row.values[map.latitude]
        );

        (void)fputc(',', output);

        write_csv_field(
            output,
            row.values[map.longitude]
        );

        (void)fputc(',', output);

        write_csv_field(
            output,
            row.values[map.depth]
        );

        (void)fputc(',', output);

        write_csv_field(
            output,
            row.values[map.magnitude]
        );

        (void)fputc(',', output);

        {
            char normalized_region[64];

            normalize_region(
                row.values[map.region],
                normalized_region,
                sizeof(normalized_region)
            );

            write_csv_field(
                output,
                normalized_region
            );
        }

        (void)fputc('\n', output);

        output_rows++;
    }

    if (ferror(input))
    {
        (void)fprintf(
            stderr,
            "Error while reading input catalog.\n"
        );

        (void)fclose(output);
        (void)fclose(input);

        return 0;
    }

    if (fclose(output) != 0)
    {
        (void)fprintf(
            stderr,
            "Error while closing output catalog.\n"
        );

        (void)fclose(input);
        return 0;
    }

    if (fclose(input) != 0)
    {
        (void)fprintf(
            stderr,
            "Error while closing input catalog.\n"
        );

        return 0;
    }

    (void)printf(
        "input_rows=%zu\n",
        input_rows
    );

    (void)printf(
        "normalized_rows=%zu\n",
        output_rows
    );

    (void)printf(
        "rejected_rows=%zu\n",
        rejected_rows
    );

    (void)printf(
        "output_file=%s\n",
        output_path
    );

    return 1;
}

static void print_usage(
    const char *program_name
)
{
    (void)fprintf(
        stderr,
        "Usage:\n"
        "  %s <usgs-raw.csv> <cameff-normalized.csv>\n",
        program_name
    );
}

int main(
    int argc,
    char **argv
)
{
    if (argc != 3)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (!normalize_catalog(
            argv[1],
            argv[2]))
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}