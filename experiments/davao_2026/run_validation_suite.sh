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
WINDOW_DIRECTORY="${RESULT_DIRECTORY}/windows"
SUMMARY_FILE="${RESULT_DIRECTORY}/davao_validation_windows.csv"

ANALYSIS_LATITUDE="5.599"
ANALYSIS_LONGITUDE="125.056"
ANALYSIS_RADIUS_KM="500"

LOOKBACK_DAYS="365"
MINIMUM_EVENTS="20"

FROZEN_THRESHOLD="0.73"

if [[ ! -x "${RUNNER}" ]]; then
    echo "Hindcast runner not found:" >&2
    echo "  ${RUNNER}" >&2
    exit 1
fi

if [[ ! -f "${CATALOG}" ]]; then
    echo "Normalized catalog not found:" >&2
    echo "  ${CATALOG}" >&2
    exit 1
fi

mkdir -p "${WINDOW_DIRECTORY}"

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

classify_score()
{
    local score="$1"

    awk \
        -v score="${score}" \
        -v threshold="${FROZEN_THRESHOLD}" \
        'BEGIN {
            if ((score + 0.0) >= (threshold + 0.0)) {
                print "1"
            } else {
                print "0"
            }
        }'
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
    local predicted_label

    observation_start_epoch="$(
        date -u -d "${observation_start_time}" +%s
    )"

    cutoff_epoch="$(
        date -u -d "${cutoff_time}" +%s
    )"

    target_epoch="$(
        date -u -d "${target_time}" +%s
    )"

    report_file="${WINDOW_DIRECTORY}/${case_id}.txt"

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

    predicted_label="$(
        classify_score "${hazard_evidence}"
    )"

    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
        "${case_id}" \
        "${label}" \
        "${predicted_label}" \
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
case_id,label,predicted_label,observation_start,cutoff,evidence_event_count,hazard_evidence,fused_confidence,dominant_pattern,dominant_expert
EOF

run_window \
    "davao_2022_control" \
    "0" \
    "2021-06-06T23:37:41Z" \
    "2022-06-06T23:37:41Z" \
    "2022-06-07T23:37:41Z" \
    "0.0" \
    "0.0" \
    "Davao 2022 negative control"

run_window \
    "davao_2023_control" \
    "0" \
    "2022-06-06T23:37:41Z" \
    "2023-06-06T23:37:41Z" \
    "2023-06-07T23:37:41Z" \
    "0.0" \
    "0.0" \
    "Davao 2023 negative control"

run_window \
    "davao_2024_control" \
    "0" \
    "2023-06-06T23:37:41Z" \
    "2024-06-06T23:37:41Z" \
    "2024-06-07T23:37:41Z" \
    "0.0" \
    "0.0" \
    "Davao 2024 negative control"

run_window \
    "davao_2025_control" \
    "0" \
    "2024-06-06T23:37:41Z" \
    "2025-06-06T23:37:41Z" \
    "2025-06-07T23:37:41Z" \
    "0.0" \
    "0.0" \
    "Davao 2025 negative control"

run_window \
    "davao_2026_positive" \
    "1" \
    "2025-06-06T23:37:41Z" \
    "2026-06-06T23:37:41Z" \
    "2026-06-07T23:37:41Z" \
    "57.0" \
    "7.8" \
    "Southern Mindanao Offshore Sarangani Philippines"

echo
echo "Validation suite complete."
echo "Frozen threshold: ${FROZEN_THRESHOLD}"
echo "Summary:"
echo "  ${SUMMARY_FILE}"