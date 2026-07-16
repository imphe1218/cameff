BEGIN {
    FS = ","
    OFS = ","

    pi = atan2(0, -1)
    earth_radius_km = 6371.0088
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

function haversine(lat1, lon1, lat2, lon2, dlat, dlon, a, c) {
    lat1 = radians(lat1)
    lon1 = radians(lon1)
    lat2 = radians(lat2)
    lon2 = radians(lon2)

    dlat = lat2 - lat1
    dlon = lon2 - lon1

    a = sin(dlat / 2.0) ^ 2 + cos(lat1) * cos(lat2) * sin(dlon / 2.0) ^ 2

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
            print "ERROR: Missing USGS CSV column: " required[i] > "/dev/stderr"
            exit 2
        }
    }

    next
}

{
    event_time = clean($(header["time"]))
    event_epoch = iso_epoch(event_time)

    if (event_epoch <= decision_epoch || event_epoch > outcome_end_epoch) {
        next
    }

    magnitude = clean($(header["mag"])) + 0.0

    if (magnitude < target_mag) {
        next
    }

    latitude = clean($(header["latitude"])) + 0.0
    longitude = clean($(header["longitude"])) + 0.0

    distance = haversine(center_lat, center_lon, latitude, longitude)

    if (distance > radius_km) {
        next
    }

    event_id = clean($(header["id"]))

    printf "%s,%s,%s,%.3f,%.6f,%.6f,%.3f\n", case_id, event_id, event_time, magnitude, latitude, longitude, distance
}
