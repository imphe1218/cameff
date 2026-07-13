#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAMEFF_SWEEP_LINE_CAPACITY 2048U
#define CAMEFF_SWEEP_MAX_CASES 1024U

typedef struct
{
    int label;
    double score;
} ValidationCase;

static int parse_label(
    const char *text,
    int *label
)
{
    char *end;
    long value;

    if (text == NULL || label == NULL)
    {
        return 0;
    }

    errno = 0;
    end = NULL;

    value = strtol(text, &end, 10);

    if (errno != 0 ||
        end == text ||
        *end != '\0' ||
        (value != 0L && value != 1L))
    {
        return 0;
    }

    *label = (int)value;

    return 1;
}

static int parse_score(
    const char *text,
    double *score
)
{
    char *end;
    double value;

    if (text == NULL || score == NULL)
    {
        return 0;
    }

    errno = 0;
    end = NULL;

    value = strtod(text, &end);

    if (errno != 0 ||
        end == text ||
        *end != '\0' ||
        !isfinite(value) ||
        value < 0.0 ||
        value > 1.0)
    {
        return 0;
    }

    *score = value;

    return 1;
}

static int parse_validation_row(
    char *line,
    ValidationCase *validation_case
)
{
    char *token;
    size_t column;

    if (line == NULL || validation_case == NULL)
    {
        return 0;
    }

    column = 0U;
    token = strtok(line, ",");

    while (token != NULL)
    {
        if (column == 1U)
        {
            if (!parse_label(
                    token,
                    &validation_case->label))
            {
                return 0;
            }
        }
        else if (column == 5U)
        {
            if (!parse_score(
                    token,
                    &validation_case->score))
            {
                return 0;
            }
        }

        column++;
        token = strtok(NULL, ",");
    }

    return column >= 6U;
}

static double safe_divide(
    double numerator,
    double denominator
)
{
    if (denominator == 0.0)
    {
        return 0.0;
    }

    return numerator / denominator;
}

static void run_threshold_sweep(
    const ValidationCase *cases,
    size_t case_count
)
{
    unsigned threshold_index;

    (void)printf(
        "threshold,tp,fp,tn,fn,"
        "precision,recall,specificity,"
        "false_positive_rate,f1,balanced_accuracy\n"
    );

    for (threshold_index = 0U;
         threshold_index <= 100U;
         threshold_index += 5U)
    {
        double threshold;

        size_t true_positive;
        size_t false_positive;
        size_t true_negative;
        size_t false_negative;

        size_t index;

        double precision;
        double recall;
        double specificity;
        double false_positive_rate;
        double f1;
        double balanced_accuracy;

        threshold =
            (double)threshold_index / 100.0;

        true_positive = 0U;
        false_positive = 0U;
        true_negative = 0U;
        false_negative = 0U;

        for (index = 0U;
             index < case_count;
             index++)
        {
            int predicted_positive;

            predicted_positive =
                cases[index].score >= threshold;

            if (cases[index].label == 1 &&
                predicted_positive != 0)
            {
                true_positive++;
            }
            else if (cases[index].label == 0 &&
                     predicted_positive != 0)
            {
                false_positive++;
            }
            else if (cases[index].label == 0)
            {
                true_negative++;
            }
            else
            {
                false_negative++;
            }
        }

        precision = safe_divide(
            (double)true_positive,
            (double)(true_positive + false_positive)
        );

        recall = safe_divide(
            (double)true_positive,
            (double)(true_positive + false_negative)
        );

        specificity = safe_divide(
            (double)true_negative,
            (double)(true_negative + false_positive)
        );

        false_positive_rate =
            1.0 - specificity;

        f1 = safe_divide(
            2.0 * precision * recall,
            precision + recall
        );

        balanced_accuracy =
            0.5 * (recall + specificity);

        (void)printf(
            "%.2f,%zu,%zu,%zu,%zu,"
            "%.6f,%.6f,%.6f,"
            "%.6f,%.6f,%.6f\n",
            threshold,
            true_positive,
            false_positive,
            true_negative,
            false_negative,
            precision,
            recall,
            specificity,
            false_positive_rate,
            f1,
            balanced_accuracy
        );
    }
}

int main(
    int argc,
    char **argv
)
{
    FILE *input;

    char line[CAMEFF_SWEEP_LINE_CAPACITY];

    ValidationCase cases[CAMEFF_SWEEP_MAX_CASES];

    size_t case_count;
    size_t line_number;

    if (argc != 2)
    {
        (void)fprintf(
            stderr,
            "Usage:\n"
            "  %s <validation-windows.csv>\n",
            argv[0]
        );

        return EXIT_FAILURE;
    }

    input = fopen(argv[1], "r");

    if (input == NULL)
    {
        (void)fprintf(
            stderr,
            "Unable to open validation file: %s\n",
            argv[1]
        );

        return EXIT_FAILURE;
    }

    if (fgets(
            line,
            sizeof(line),
            input) == NULL)
    {
        (void)fprintf(
            stderr,
            "Validation file is empty.\n"
        );

        (void)fclose(input);
        return EXIT_FAILURE;
    }

    case_count = 0U;
    line_number = 1U;

    while (fgets(
            line,
            sizeof(line),
            input) != NULL)
    {
        size_t length;

        line_number++;

        length = strlen(line);

        while (length > 0U &&
               (line[length - 1U] == '\n' ||
                line[length - 1U] == '\r'))
        {
            line[length - 1U] = '\0';
            length--;
        }

        if (line[0] == '\0')
        {
            continue;
        }

        if (case_count >=
            CAMEFF_SWEEP_MAX_CASES)
        {
            (void)fprintf(
                stderr,
                "Too many validation cases.\n"
            );

            (void)fclose(input);
            return EXIT_FAILURE;
        }

        if (!parse_validation_row(
                line,
                &cases[case_count]))
        {
            (void)fprintf(
                stderr,
                "Invalid validation row at line %zu.\n",
                line_number
            );

            (void)fclose(input);
            return EXIT_FAILURE;
        }

        case_count++;
    }

    if (fclose(input) != 0)
    {
        (void)fprintf(
            stderr,
            "Unable to close validation file.\n"
        );

        return EXIT_FAILURE;
    }

    if (case_count == 0U)
    {
        (void)fprintf(
            stderr,
            "No validation cases were loaded.\n"
        );

        return EXIT_FAILURE;
    }

    run_threshold_sweep(
        cases,
        case_count
    );

    return EXIT_SUCCESS;
}