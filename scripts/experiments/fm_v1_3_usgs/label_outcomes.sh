#!/usr/bin/env bash

set -euo pipefail

PROJECT_ROOT="$(
    cd "$(dirname "${BASH_SOURCE[0]}")/../../.." &&
    pwd
)"

EXPERIMENT_ROOT="${PROJECT_ROOT}/experiments/fm_v1_3_wider_usgs_hindcast"
REGISTRY_FILE="${EXPERIMENT_ROOT}/experiment_registry.csv"
DATASET_MAP_FILE="${EXPERIMENT_ROOT}/dataset_map.csv"
RESULTS_DIRECTORY="${EXPERIMENT_ROOT}/results"

AWK_PROGRAM="${PROJECT_ROOT}/scripts/experiments/fm_v1_3_usgs/label_outcomes.awk"
OUTPUT_FILE="${RESULTS_DIRECTORY}/outcome_labels.csv"
DETAIL_FILE="${RESULTS_DIRECTORY}/qualifying_events.csv"

: "${CAMEFF_CATALOG_REPO:?Set CAMEFF_CATALOG_REPO to the catalog repository path}"

for command_name in gawk date sort head cut wc mktemp; do
    command -v "${command_name}" >/dev/null 2>&1 || {
        printf 'ERROR: Required command not found: %s\n' \
            "${command_name}" >&2
        exit 1
    }
done

for required_file in \
    "${REGISTRY_FILE}" \
    "${DATASET_MAP_FILE}" \
    "${AWK_PROGRAM}"
do
    [[ -f "${required_file}" ]] || {
        printf 'ERROR: Required file not found:\n  %s\n' \
            "${required_file}" >&2
        exit 1
    }
done

mkdir -p "${RESULTS_DIRECTORY}"

declare -A OUTCOME_PATHS

while IFS=',' read -r \
    dataset_id \
    feature_relative_path \
    outcome_relative_path
do
    dataset_id="${dataset_id%$'\r'}"
    outcome_relative_path="${outcome_relative_path%$'\r'}"

    [[ "${dataset_id}" == "catalog_dataset" ]] && continue
    [[ -z "${dataset_id}" ]] && continue

    OUTCOME_PATHS["${dataset_id}"]="${outcome_relative_path}"
done < "${DATASET_MAP_FILE}"

printf '%s\n' \
'case_id,case_role,decision_time_utc,outcome_end_utc,target_magnitude,radius_km,qualifying_event_count,largest_qualifying_magnitude,first_qualifying_event_id,first_qualifying_event_time,outcome_label' \
> "${OUTPUT_FILE}"

printf '%s\n' \
'case_id,event_id,event_time_utc,magnitude,latitude,longitude,distance_km' \
> "${DETAIL_FILE}"

temporary_directory="$(mktemp -d)"

cleanup() {
    rm -rf "${temporary_directory}"
}

trap cleanup EXIT

while IFS=',' read -r \
    case_id \
    case_role \
    region_name \
    tectonic_regime \
    catalog_dataset \
    target_event_id \
    target_time_utc \
    decision_time_utc \
    lookback_start_utc \
    center_latitude \
    center_longitude \
    radius_km \
    min_catalog_magnitude \
    target_magnitude \
    forecast_horizon_days \
    analysis_stage \
    declared_math_path \
    existing_outcome_label \
    case_status
do
    case_id="${case_id%$'\r'}"
    case_role="${case_role%$'\r'}"
    catalog_dataset="${catalog_dataset%$'\r'}"
    decision_time_utc="${decision_time_utc%$'\r'}"
    center_latitude="${center_latitude%$'\r'}"
    center_longitude="${center_longitude%$'\r'}"
    radius_km="${radius_km%$'\r'}"
    target_magnitude="${target_magnitude%$'\r'}"
    forecast_horizon_days="${forecast_horizon_days%$'\r'}"

    [[ "${case_id}" == "case_id" ]] && continue
    [[ -z "${case_id}" ]] && continue

    outcome_relative_path="${OUTCOME_PATHS[${catalog_dataset}]:-}"

    [[ -n "${outcome_relative_path}" ]] || {
        printf 'ERROR: No outcome path for dataset: %s\n' \
            "${catalog_dataset}" >&2
        exit 1
    }

    outcome_catalog="${CAMEFF_CATALOG_REPO}/${outcome_relative_path}"

    [[ -f "${outcome_catalog}" ]] || {
        printf 'ERROR: Outcome catalog missing:\n  %s\n' \
            "${outcome_catalog}" >&2
        exit 1
    }

    decision_epoch="$(date -u --date="${decision_time_utc}" +%s)"

    outcome_end_epoch="$(
        date -u \
            --date="${decision_time_utc} +${forecast_horizon_days} days" \
            +%s
    )"

    outcome_end_utc="$(
        date -u \
            --date="@${outcome_end_epoch}" \
            '+%Y-%m-%dT%H:%M:%SZ'
    )"

    case_detail_file="${temporary_directory}/${case_id}.csv"

    TZ=UTC gawk \
        -v case_id="${case_id}" \
        -v decision_epoch="${decision_epoch}" \
        -v outcome_end_epoch="${outcome_end_epoch}" \
        -v center_lat="${center_latitude}" \
        -v center_lon="${center_longitude}" \
        -v radius_km="${radius_km}" \
        -v target_mag="${target_magnitude}" \
        -f "${AWK_PROGRAM}" \
        "${outcome_catalog}" \
        > "${case_detail_file}"

    qualifying_count="$(wc -l < "${case_detail_file}")"

    largest_magnitude=""
    first_event_id=""
    first_event_time=""

    if (( qualifying_count > 0 )); then
        sort -t',' -k3,3 "${case_detail_file}" \
            >> "${DETAIL_FILE}"

        first_event_id="$(
            sort -t',' -k3,3 "${case_detail_file}" |
            head -n 1 |
            cut -d',' -f2
        )"

        first_event_time="$(
            sort -t',' -k3,3 "${case_detail_file}" |
            head -n 1 |
            cut -d',' -f3
        )"

        largest_magnitude="$(
            sort -t',' -k4,4nr "${case_detail_file}" |
            head -n 1 |
            cut -d',' -f4
        )"
    fi

    case "${case_role}" in
        known_positive)
            if (( qualifying_count > 0 )); then
                outcome_label="positive_confirmed"
            else
                outcome_label="positive_target_not_found"
            fi
            ;;

        deterministic_control)
            if (( qualifying_count > 0 )); then
                outcome_label="control_positive"
            else
                outcome_label="control_negative"
            fi
            ;;

        *)
            outcome_label="unknown_role"
            ;;
    esac

    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
        "${case_id}" \
        "${case_role}" \
        "${decision_time_utc}" \
        "${outcome_end_utc}" \
        "${target_magnitude}" \
        "${radius_km}" \
        "${qualifying_count}" \
        "${largest_magnitude}" \
        "${first_event_id}" \
        "${first_event_time}" \
        "${outcome_label}" \
        >> "${OUTPUT_FILE}"

    printf '%-25s %-26s qualifying=%s\n' \
        "${case_id}" \
        "${outcome_label}" \
        "${qualifying_count}"

done < "${REGISTRY_FILE}"

printf '\nOutcome labeling complete.\n'
printf 'Summary: %s\n' "${OUTPUT_FILE}"
printf 'Events:  %s\n' "${DETAIL_FILE}"
