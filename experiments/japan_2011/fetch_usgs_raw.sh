#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(
    cd "$(dirname "${BASH_SOURCE[0]}")"
    pwd
)"

RAW_DATA_DIR="${SCRIPT_DIR}/data/raw"
OUTPUT_FILE="${RAW_DATA_DIR}/usgs_japan_2010_2011_raw.csv"

mkdir -p "${RAW_DATA_DIR}"

USGS_QUERY_URL="https://earthquake.usgs.gov/fdsnws/event/1/query"

TARGET_LATITUDE="38.297"
TARGET_LONGITUDE="142.373"
ANALYSIS_RADIUS_KM="500"

START_TIME="2010-03-11T05:46:24Z"

# One second before the target earthquake prevents the target
# from entering the downloaded precursor catalog.
END_TIME="2011-03-11T05:46:23Z"

MINIMUM_MAGNITUDE="2.5"

echo "Downloading USGS Japan precursor catalog..."
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

LINE_COUNT="$(wc -l < "${OUTPUT_FILE}")"

echo "Download complete."
echo "Rows including header: ${LINE_COUNT}"
echo "Raw catalog: ${OUTPUT_FILE}"