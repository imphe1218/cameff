#include "cameff/geo.h"

#include <math.h>

static double degrees_to_radians(double degrees) {
    return degrees * (3.14159265358979323846 / 180.0);
}

double cameff_haversine_km(double latitude1_deg, double longitude1_deg,
                           double latitude2_deg, double longitude2_deg) {
    const double earth_radius_km = 6371.0088;
    const double latitude1 = degrees_to_radians(latitude1_deg);
    const double latitude2 = degrees_to_radians(latitude2_deg);
    const double delta_latitude = latitude2 - latitude1;
    const double delta_longitude = degrees_to_radians(longitude2_deg - longitude1_deg);
    const double sin_latitude = sin(delta_latitude / 2.0);
    const double sin_longitude = sin(delta_longitude / 2.0);
    const double a = (sin_latitude * sin_latitude) +
                     (cos(latitude1) * cos(latitude2) * sin_longitude * sin_longitude);
    const double bounded_a = fmin(1.0, fmax(0.0, a));
    return 2.0 * earth_radius_km * asin(sqrt(bounded_a));
}
