#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(
    cd "$(dirname "${BASH_SOURCE[0]}")"
    pwd
)"

RAW_DATA_DIRECTORY="${SCRIPT_DIR}/data/raw"
OUTPUT_FILE="${RAW_DATA_DIRECTORY}/usgs_cebu_2020_2025_raw.csv"

mkdir -p "${RAW_DATA_DIRECTORY}"

USGS_QUERY_URL="https://earthquake.usgs.gov/fdsnws/event/1/query"

TARGET_LATITUDE="11.156"
TARGET_LONGITUDE="124.111"

ANALYSIS_RADIUS_KM="500"

START_TIME="2020-09-29T13:59:43Z"

# One second before the target earthquake.
END_TIME="2025-09-30T13:59:42Z"

MINIMUM_MAGNITUDE="2.5"

echo "Downloading USGS Cebu validation catalog..."
echo "Output: ${OUTPUT_FILE}"

curl \
    --fail \
    --location \
    --silent \
    --show-error \
    --get \
    "${USGS_QUERY_URL}" \
    --data-urlencode "format=csv" \
    --data-urlencode "starttime=${START_TIME}" \
    --data-urlencode "endtime=${END_TIME}" \
    --data-urlencode "latitude=${TARGET_LATITUDE}" \
    --data-urlencode "longitude=${TARGET_LONGITUDE}" \
    --data-urlencode "maxradiuskm=${ANALYSIS_RADIUS_KM}" \
    --data-urlencode "minmagnitude=${MINIMUM_MAGNITUDE}" \
    --data-urlencode "eventtype=earthquake" \
    --data-urlencode "orderby=time-asc" \
    --output "${OUTPUT_FILE}"

if [[ ! -s "${OUTPUT_FILE}" ]]; then
    echo "USGS returned an empty catalog." >&2
    exit 1
fi

LINE_COUNT="$(
    wc -l < "${OUTPUT_FILE}"
)"

if (( LINE_COUNT >= 20001 )); then
    echo "Catalog may have reached the USGS query limit." >&2
    echo "Rows including header: ${LINE_COUNT}" >&2
    echo "The request must be divided into smaller intervals." >&2
    exit 1
fi

echo "Download complete."
echo "Rows including header: ${LINE_COUNT}"
echo "Raw catalog: ${OUTPUT_FILE}"