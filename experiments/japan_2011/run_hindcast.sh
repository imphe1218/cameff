#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(
    cd "$(dirname "${BASH_SOURCE[0]}")"
    pwd
)"

REPOSITORY_ROOT="$(
    cd "${SCRIPT_DIR}/../.."
    pwd
)"

CATALOG="${SCRIPT_DIR}/data/normalized/catalog.csv"
RUNNER="${REPOSITORY_ROOT}/build/cameff_hindcast_runner"
REPORT_DIRECTORY="${SCRIPT_DIR}/results"
REPORT_FILE="${REPORT_DIRECTORY}/japan_2011_hindcast.txt"

TARGET_TIME="2011-03-11T05:46:24Z"
OBSERVATION_START_TIME="2010-03-10T05:46:24Z"
CUTOFF_TIME="2011-03-10T05:46:24Z"

TARGET_EPOCH="$(
    date -u -d "${TARGET_TIME}" +%s
)"

OBSERVATION_START_EPOCH="$(
    date -u -d "${OBSERVATION_START_TIME}" +%s
)"

CUTOFF_EPOCH="$(
    date -u -d "${CUTOFF_TIME}" +%s
)"

TARGET_LATITUDE="38.297"
TARGET_LONGITUDE="142.373"
TARGET_DEPTH_KM="29.0"
TARGET_MAGNITUDE="9.1"
TARGET_REGION="Near the east coast of Honshu, Japan"

ANALYSIS_LATITUDE="38.297"
ANALYSIS_LONGITUDE="142.373"
ANALYSIS_RADIUS_KM="500"
LOOKBACK_DAYS="365"
MINIMUM_EVENTS="20"

if [[ ! -x "${RUNNER}" ]]; then
    echo "Hindcast runner not found: ${RUNNER}" >&2
    echo "Build the project first:" >&2
    echo "  cmake --build build" >&2
    exit 1
fi

if [[ ! -f "${CATALOG}" ]]; then
    echo "Normalized catalog not found: ${CATALOG}" >&2
    echo "Run these commands first:" >&2
    echo "  ./experiments/japan_2011/fetch_usgs_raw.sh" >&2
    echo "  ./experiments/japan_2011/normalize_catalog.sh" >&2
    exit 1
fi

mkdir -p "${REPORT_DIRECTORY}"

"${RUNNER}" \
    "${CATALOG}" \
    "${TARGET_EPOCH}" \
    "${TARGET_LATITUDE}" \
    "${TARGET_LONGITUDE}" \
    "${TARGET_DEPTH_KM}" \
    "${TARGET_MAGNITUDE}" \
    "${TARGET_REGION}" \
    "${OBSERVATION_START_EPOCH}" \
    "${CUTOFF_EPOCH}" \
    "${ANALYSIS_LATITUDE}" \
    "${ANALYSIS_LONGITUDE}" \
    "${ANALYSIS_RADIUS_KM}" \
    "${LOOKBACK_DAYS}" \
    "${MINIMUM_EVENTS}" \
    | tee "${REPORT_FILE}"

echo
echo "Hindcast report written to:"
echo "  ${REPORT_FILE}"