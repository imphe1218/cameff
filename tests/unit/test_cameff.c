#include "cameff.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void) {
    double p = 0.168235;
    assert(fabs(cameff_sigmoid(cameff_probability_logit(p)) - p) < 1e-12);
    puts("C17 smoke test passed");
    return 0;
}
