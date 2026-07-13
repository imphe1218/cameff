#include "../../src/validation/metrics.h"

#include <assert.h>


int main()
{

    ClassificationMetrics m =
    {
        .true_positive = 80,
        .false_positive = 20,
        .true_negative = 900,
        .false_negative = 10
    };


    double p = precision(&m);
    double r = recall(&m);


    assert(p > 0.79);
    assert(r > 0.88);


    return 0;
}