#include "hindcast.h"


HindcastResult run_hindcast(
        HindcastExperiment *experiment
)
{

    HindcastResult result;


    /*
     * Future implementation:
     *
     * 1. Load historical signals
     * 2. Execute CAMEFF experts
     * 3. Fuse probabilities
     */


    result.hazard_score = 0.0;
    result.confidence = 0.0;

    result.expert[0] = '\0';


    return result;
}