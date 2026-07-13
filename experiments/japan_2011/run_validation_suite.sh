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
DETAIL_DIRECTORY="${RESULT_DIRECTORY}/windows"
SUMMARY_FILE="${RESULT_DIRECTORY}/japan_validation_windows.csv"

ANALYSIS_LATITUDE="38.297"
ANALYSIS_LONGITUDE="142.373"
ANALYSIS_RADIUS_KM="500"

LOOKBACK_DAYS="365"
MINIMUM_EVENTS="20"

mkdir -p "${DETAIL_DIRECTORY}"

if [[ ! -x "${RUNNER}" ]]; then
    echo "Hindcast runner not found: ${RUNNER}" >&2
    echo "Build the project first:" >&2
    echo "  cmake --build build" >&2
    exit 1
fi

if [[ ! -f "${CATALOG}" ]]; then
    echo "Normalized catalog not found: ${CATALOG}" >&2
    echo "Run the acquisition and normalization scripts first." >&2
    exit 1
fi

extract_value()
{
    local key="$1"
    local file="$2"

    awk -F= -v requested_key="${key}" '
        $1 == requested_key {
            print substr($0, index($0, "=") + 1)
            exit
        }
    ' "${file}"
}

run_window()
{
    local case_id="$1"
    local label="$2"
    local observation_start_time="$3"
    local cutoff_time="$4"

    local target_time="$5"
    local target_depth="$6"
    local target_magnitude="$7"
    local target_region="$8"

    local observation_start_epoch
    local cutoff_epoch
    local target_epoch

    local report_file

    local experiment_status
    local evidence_event_count
    local hazard_evidence
    local fused_confidence
    local dominant_pattern
    local dominant_expert

    observation_start_epoch="$(
        date -u -d "${observation_start_time}" +%s
    )"

    cutoff_epoch="$(
        date -u -d "${cutoff_time}" +%s
    )"

    target_epoch="$(
        date -u -d "${target_time}" +%s
    )"

    report_file="${DETAIL_DIRECTORY}/${case_id}.txt"

    echo "Running ${case_id}..."

    "${RUNNER}" \
        "${CATALOG}" \
        "${target_epoch}" \
        "${ANALYSIS_LATITUDE}" \
        "${ANALYSIS_LONGITUDE}" \
        "${target_depth}" \
        "${target_magnitude}" \
        "${target_region}" \
        "${observation_start_epoch}" \
        "${cutoff_epoch}" \
        "${ANALYSIS_LATITUDE}" \
        "${ANALYSIS_LONGITUDE}" \
        "${ANALYSIS_RADIUS_KM}" \
        "${LOOKBACK_DAYS}" \
        "${MINIMUM_EVENTS}" \
        > "${report_file}"

    experiment_status="$(
        extract_value "experiment_status" "${report_file}"
    )"

    if [[ "${experiment_status}" != "ok" ]]; then
        echo "Experiment failed for ${case_id}." >&2
        cat "${report_file}" >&2
        exit 1
    fi

    evidence_event_count="$(
        extract_value "evidence_event_count" "${report_file}"
    )"

    hazard_evidence="$(
        extract_value "hazard_evidence" "${report_file}"
    )"

    fused_confidence="$(
        extract_value "fused_confidence" "${report_file}"
    )"

    dominant_pattern="$(
        extract_value "dominant_pattern" "${report_file}"
    )"

    dominant_expert="$(
        extract_value "dominant_expert" "${report_file}"
    )"

    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
        "${case_id}" \
        "${label}" \
        "${observation_start_time}" \
        "${cutoff_time}" \
        "${evidence_event_count}" \
        "${hazard_evidence}" \
        "${fused_confidence}" \
        "${dominant_pattern}" \
        "${dominant_expert}" \
        >> "${SUMMARY_FILE}"
}

cat > "${SUMMARY_FILE}" <<'EOF'
case_id,label,observation_start,cutoff,evidence_event_count,hazard_evidence,fused_confidence,dominant_pattern,dominant_expert
EOF

#
# Negative controls use a reporting anchor 24 hours after the cutoff.
# The target fields are not passed into the CAMEFF signal evaluator.
#
run_window \
    "japan_2007_control" \
    "0" \
    "2006-03-10T05:46:24Z" \
    "2007-03-10T05:46:24Z" \
    "2007-03-11T05:46:24Z" \
    "0.0" \
    "0.0" \
    "Japan 2007 negative control"

run_window \
    "japan_2008_control" \
    "0" \
    "2007-03-10T05:46:24Z" \
    "2008-03-10T05:46:24Z" \
    "2008-03-11T05:46:24Z" \
    "0.0" \
    "0.0" \
    "Japan 2008 negative control"

run_window \
    "japan_2009_control" \
    "0" \
    "2008-03-10T05:46:24Z" \
    "2009-03-10T05:46:24Z" \
    "2009-03-11T05:46:24Z" \
    "0.0" \
    "0.0" \
    "Japan 2009 negative control"

run_window \
    "japan_2010_control" \
    "0" \
    "2009-03-10T05:46:24Z" \
    "2010-03-10T05:46:24Z" \
    "2010-03-11T05:46:24Z" \
    "0.0" \
    "0.0" \
    "Japan 2010 negative control"

run_window \
    "japan_2011_positive" \
    "1" \
    "2010-03-10T05:46:24Z" \
    "2011-03-10T05:46:24Z" \
    "2011-03-11T05:46:24Z" \
    "29.0" \
    "9.1" \
    "Near the east coast of Honshu Japan"

echo
echo "Validation suite complete."
echo "Summary:"
echo "  ${SUMMARY_FILE}"