BEGIN {
    FS = ","
    OFS = ","

    pi = atan2(0, -1)
    earth_radius_km = 6371.0088

    total_count = 0

    recent_count = 0
    background_count = 0

    recent_mag_count = 0
    previous_mag_count = 0

    migration_count = 0

    b_recent_count = 0
    b_background_count = 0

    recent_max_mag = ""
    previous_max_mag = ""

    recent_distance_sum = 0.0
    recent_min_distance = ""

    migration_sum_x = 0.0
    migration_sum_y = 0.0
    migration_sum_xx = 0.0
    migration_sum_xy = 0.0

    b_recent_mag_sum = 0.0
    b_background_mag_sum = 0.0

    leakage_count = 0
}

function clean(value) {
    sub(/\r$/, "", value)
    sub(/^"/, "", value)
    sub(/"$/, "", value)

    return value
}

function iso_epoch(value, pieces, date_part, time_part) {
    value = clean(value)

    sub(/\.[0-9]+Z$/, "", value)
    sub(/Z$/, "", value)

    split(value, pieces, "T")

    date_part = pieces[1]
    time_part = pieces[2]

    gsub(/-/, " ", date_part)
    gsub(/:/, " ", time_part)

    return mktime(date_part " " time_part, 1)
}

function radians(degrees) {
    return degrees * pi / 180.0
}

function haversine(lat1, lon1, lat2, lon2, rlat1, rlon1, rlat2, rlon2, dlat, dlon, a, c) {
    rlat1 = radians(lat1)
    rlon1 = radians(lon1)
    rlat2 = radians(lat2)
    rlon2 = radians(lon2)

    dlat = rlat2 - rlat1
    dlon = rlon2 - rlon1

    a = sin(dlat / 2.0) ^ 2 + cos(rlat1) * cos(rlat2) * sin(dlon / 2.0) ^ 2

    if (a > 1.0) {
        a = 1.0
    }

    if (a < 0.0) {
        a = 0.0
    }

    c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a))

    return earth_radius_km * c
}

NR == 1 {
    for (i = 1; i <= NF; i++) {
        header[clean($i)] = i
    }

    required[1] = "time"
    required[2] = "latitude"
    required[3] = "longitude"
    required[4] = "mag"
    required[5] = "id"

    for (i = 1; i <= 5; i++) {
        if (!(required[i] in header)) {
            print "ERROR: Missing USGS column: " required[i] > "/dev/stderr"
            exit 2
        }
    }

    next
}

{
    event_time = clean($(header["time"]))
    event_epoch = iso_epoch(event_time)

    if (event_epoch >= decision_epoch) {
        leakage_count++
        next
    }

    if (event_epoch < lookback_start_epoch) {
        next
    }

    latitude = clean($(header["latitude"])) + 0.0
    longitude = clean($(header["longitude"])) + 0.0
    magnitude = clean($(header["mag"])) + 0.0

    if (magnitude < catalog_min_magnitude) {
        next
    }

    distance_km = haversine(center_lat, center_lon, latitude, longitude)

    if (distance_km > radius_km) {
        next
    }

    total_count++

    if (event_epoch >= recent_start_epoch) {
        recent_count++
        recent_distance_sum += distance_km

        if (recent_min_distance == "" || distance_km < recent_min_distance) {
            recent_min_distance = distance_km
        }
    }

    if (event_epoch >= background_start_epoch && event_epoch < recent_start_epoch) {
        background_count++
    }

    if (event_epoch >= magnitude_start_epoch) {
        recent_mag_count++

        if (recent_max_mag == "" || magnitude > recent_max_mag) {
            recent_max_mag = magnitude
        }
    }

    if (event_epoch >= previous_magnitude_start_epoch && event_epoch < magnitude_start_epoch) {
        previous_mag_count++

        if (previous_max_mag == "" || magnitude > previous_max_mag) {
            previous_max_mag = magnitude
        }
    }

    if (event_epoch >= migration_start_epoch) {
        x_days = (event_epoch - migration_start_epoch) / 86400.0
        y_distance = distance_km

        migration_count++
        migration_sum_x += x_days
        migration_sum_y += y_distance
        migration_sum_xx += x_days * x_days
        migration_sum_xy += x_days * y_distance
    }

    if (event_epoch >= recent_b_start_epoch) {
        b_recent_count++
        b_recent_mag_sum += magnitude
    }

    if (event_epoch >= background_b_start_epoch && event_epoch < recent_b_start_epoch) {
        b_background_count++
        b_background_mag_sum += magnitude
    }
}

END {
    recent_days = recent_window_days + 0.0
    background_days = background_window_days - recent_window_days

    if (recent_days > 0.0) {
        recent_rate = recent_count / recent_days
    } else {
        recent_rate = 0.0
    }

    if (background_days > 0.0) {
        background_rate = background_count / background_days
    } else {
        background_rate = 0.0
    }

    if (background_rate > 0.0) {
        rate_ratio = recent_rate / background_rate
    } else {
        rate_ratio = ""
    }

    if (recent_count > 0) {
        recent_mean_distance = recent_distance_sum / recent_count
    } else {
        recent_mean_distance = ""
    }

    if (recent_max_mag != "" && previous_max_mag != "") {
        delta_max_magnitude = recent_max_mag - previous_max_mag
    } else {
        delta_max_magnitude = ""
    }

    denominator = migration_count * migration_sum_xx - migration_sum_x * migration_sum_x

    if (migration_count >= 2 && denominator != 0.0) {
        migration_slope = (migration_count * migration_sum_xy - migration_sum_x * migration_sum_y) / denominator
    } else {
        migration_slope = ""
    }

    if (b_recent_count > 0) {
        b_recent_mean_mag = b_recent_mag_sum / b_recent_count
    } else {
        b_recent_mean_mag = ""
    }

    if (b_background_count > 0) {
        b_background_mean_mag = b_background_mag_sum / b_background_count
    } else {
        b_background_mean_mag = ""
    }

    printf "%s,%d,%d,%.9f,%d,%.9f,", case_id, total_count, recent_count, recent_rate, background_count, background_rate

    if (rate_ratio == "") {
        printf ","
    } else {
        printf "%.9f,", rate_ratio
    }

    printf "%d,", recent_mag_count

    if (recent_max_mag == "") {
        printf ","
    } else {
        printf "%.3f,", recent_max_mag
    }

    printf "%d,", previous_mag_count

    if (previous_max_mag == "") {
        printf ","
    } else {
        printf "%.3f,", previous_max_mag
    }

    if (delta_max_magnitude == "") {
        printf ","
    } else {
        printf "%.3f,", delta_max_magnitude
    }

    if (recent_mean_distance == "") {
        printf ","
    } else {
        printf "%.6f,", recent_mean_distance
    }

    if (recent_min_distance == "") {
        printf ","
    } else {
        printf "%.6f,", recent_min_distance
    }

    printf "%d,", migration_count

    if (migration_slope == "") {
        printf ","
    } else {
        printf "%.9f,", migration_slope
    }

    printf "%d,", b_recent_count

    if (b_recent_mean_mag == "") {
        printf ","
    } else {
        printf "%.6f,", b_recent_mean_mag
    }

    printf "%d,", b_background_count

    if (b_background_mean_mag == "") {
        printf ","
    } else {
        printf "%.6f,", b_background_mean_mag
    }

    printf "%d\n", leakage_count
}
