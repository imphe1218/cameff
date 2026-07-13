#include "metrics.h"


double precision(ClassificationMetrics *m)
{
    int denominator =
        m->true_positive +
        m->false_positive;


    if(denominator == 0)
        return 0.0;


    return
        (double)m->true_positive /
        denominator;
}



double recall(ClassificationMetrics *m)
{

    int denominator =
        m->true_positive +
        m->false_negative;


    if(denominator == 0)
        return 0.0;


    return
        (double)m->true_positive /
        denominator;

}



double false_positive_rate(ClassificationMetrics *m)
{

    int denominator =
        m->false_positive +
        m->true_negative;


    if(denominator == 0)
        return 0.0;


    return
        (double)m->false_positive /
        denominator;

}