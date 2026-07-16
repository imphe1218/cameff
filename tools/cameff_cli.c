#include "cameff.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int run_etas_csv(int argc, char **argv) {
    FILE *fp;
    cameff_event_t *events = NULL;
    size_t capacity = 1024, count = 0;
    double decision_time;
    cameff_etas_fit_t fit;
    double probability = 0.0;
    char line[256];

    if (argc != 4) {
        fprintf(stderr, "usage: cameff_cli etas-csv EVENTS_CSV DECISION_TIME_DAYS\n");
        return 2;
    }

    decision_time = strtod(argv[3], NULL);
    fp = fopen(argv[2], "r");
    if (fp == NULL) {
        perror("fopen");
        return 2;
    }

    events = (cameff_event_t *)malloc(capacity * sizeof(*events));
    if (events == NULL) {
        fclose(fp);
        return 2;
    }

    if (fgets(line, sizeof(line), fp) == NULL) {
        free(events);
        fclose(fp);
        return 2;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        double t, m;
        if (sscanf(line, "%lf,%lf", &t, &m) != 2) continue;
        if (count == capacity) {
            cameff_event_t *next;
            capacity *= 2;
            next = (cameff_event_t *)realloc(events, capacity * sizeof(*events));
            if (next == NULL) {
                free(events);
                fclose(fp);
                return 2;
            }
            events = next;
        }
        events[count].time_days = t;
        events[count].magnitude = m;
        count++;
    }
    fclose(fp);

    if (cameff_fit_temporal_etas(events, count, decision_time, &fit) != CAMEFF_AVAILABLE) {
        printf("{\"status\":\"%s\"}\n", cameff_availability_name(fit.status));
        free(events);
        return 3;
    }

    if (cameff_seven_day_m6_probability(events, count, decision_time, &fit, &probability)
        != CAMEFF_AVAILABLE) {
        free(events);
        return 3;
    }

    printf(
        "{\"status\":\"AVAILABLE\",\"mc\":%.17g,\"b_value\":%.17g,"
        "\"alpha\":%.17g,\"c\":%.17g,\"p\":%.17g,\"mu\":%.17g,"
        "\"k\":%.17g,\"loglik\":%.17g,\"branching_proxy\":%.17g,"
        "\"raw_probability\":%.17g}\n",
        fit.mc, fit.b_value, fit.alpha, fit.c, fit.p, fit.mu, fit.k,
        fit.loglik, fit.branching_proxy, probability
    );

    free(events);
    return 0;
}

static int run_calibration(int argc, char **argv) {
    cameff_calibration_t c;
    double raw, out;
    cameff_availability_t status;

    if (argc != 10) {
        fprintf(stderr,
            "usage: cameff_cli calibrate RAW ROWS POS PREV INTERCEPT COEF_LOGP COEF_NEGLOG1MP STATUS\n");
        return 2;
    }

    raw = strtod(argv[2], NULL);
    memset(&c, 0, sizeof(c));
    c.training_rows = (size_t)strtoull(argv[3], NULL, 10);
    c.training_positives = (size_t)strtoull(argv[4], NULL, 10);
    c.training_prevalence = strtod(argv[5], NULL);
    c.intercept = strtod(argv[6], NULL);
    c.coef_log_p = strtod(argv[7], NULL);
    c.coef_neg_log_1mp = strtod(argv[8], NULL);
    c.status = strcmp(argv[9], "AVAILABLE") == 0
        ? CAMEFF_AVAILABLE : CAMEFF_INSUFFICIENT_EVIDENCE;

    status = cameff_apply_p29k_calibration(raw, &c, &out);
    printf("{\"status\":\"%s\",\"calibrated_probability\":%.17g}\n",
           cameff_availability_name(status),
           status == CAMEFF_AVAILABLE ? out : 0.0);
    return status == CAMEFF_AVAILABLE ? 0 : 3;
}

int main(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "calibrate") == 0)
        return run_calibration(argc, argv);
    if (argc >= 2 && strcmp(argv[1], "etas-csv") == 0)
        return run_etas_csv(argc, argv);

    fprintf(stderr, "cameff_cli: supported commands: calibrate, etas-csv\n");
    return 2;
}
