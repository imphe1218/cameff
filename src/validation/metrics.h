#ifndef METRICS_H
#define METRICS_H


typedef struct {

    int true_positive;

    int false_positive;

    int true_negative;

    int false_negative;


} ClassificationMetrics;


double precision(ClassificationMetrics *m);

double recall(ClassificationMetrics *m);

double false_positive_rate(ClassificationMetrics *m);


#endif