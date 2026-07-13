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

RESULT_DIRECTORY="${SCRIPT_DIR}/results"
REPORT_FILE="${RESULT_DIRECTORY}/cebu_2025_hindcast.txt"

TARGET_TIME="2025-09-30T13:59:43Z"

# Frozen 24-hour forecasting lead.
CUTOFF_TIME="2025-09-29T13:59:43Z"

# One-year evidence window ending at the cutoff.
OBSERVATION_START_TIME="2024-09-29T13:59:43Z"

TARGET_EPOCH="$(
    date -u -d "${TARGET_TIME}" +%s
)"

CUTOFF_EPOCH="$(
    date -u -d "${CUTOFF_TIME}" +%s
)"

OBSERVATION_START_EPOCH="$(
    date -u -d "${OBSERVATION_START_TIME}" +%s
)"

TARGET_LATITUDE="11.156"
TARGET_LONGITUDE="124.111"
TARGET_DEPTH_KM="10.0"
TARGET_MAGNITUDE="6.9"
TARGET_REGION="Offshore Northern Cebu Philippines"

ANALYSIS_LATITUDE="11.156"
ANALYSIS_LONGITUDE="124.111"
ANALYSIS_RADIUS_KM="500"

LOOKBACK_DAYS="365"
MINIMUM_EVENTS="20"

FROZEN_THRESHOLD="0.73"

if [[ ! -x "${RUNNER}" ]]; then
    echo "Hindcast runner not found:" >&2
    echo "  ${RUNNER}" >&2
    echo "Build the project first:" >&2
    echo "  cmake --build build" >&2
    exit 1
fi

if [[ ! -f "${CATALOG}" ]]; then
    echo "Normalized catalog not found:" >&2
    echo "  ${CATALOG}" >&2
    echo "Run the acquisition and normalization scripts first." >&2
    exit 1
fi

mkdir -p "${RESULT_DIRECTORY}"

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

HAZARD_EVIDENCE="$(
    awk -F= '
        $1 == "hazard_evidence" {
            print $2
            exit
        }
    ' "${REPORT_FILE}"
)"

if [[ -z "${HAZARD_EVIDENCE}" ]]; then
    echo "Unable to extract hazard evidence." >&2
    exit 1
fi

CLASSIFICATION="$(
    awk \
        -v score="${HAZARD_EVIDENCE}" \
        -v threshold="${FROZEN_THRESHOLD}" \
        'BEGIN {
            if ((score + 0.0) >= (threshold + 0.0)) {
                print "warning"
            } else {
                print "no_warning"
            }
        }'
)"

echo "frozen_threshold=${FROZEN_THRESHOLD}" \
    | tee -a "${REPORT_FILE}"

echo "threshold_classification=${CLASSIFICATION}" \
    | tee -a "${REPORT_FILE}"

echo
echo "Hindcast report written to:"
echo "  ${REPORT_FILE}"