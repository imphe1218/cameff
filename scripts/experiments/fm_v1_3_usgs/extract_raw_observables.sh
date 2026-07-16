#!/usr/bin/env bash

set -euo pipefail

PROJECT_ROOT="$(
    cd "$(dirname "${BASH_SOURCE[0]}")/../../.." &&
    pwd
)"

EXPERIMENT_ROOT="${PROJECT_ROOT}/experiments/fm_v1_3_wider_usgs_hindcast"
REGISTRY_FILE="${EXPERIMENT_ROOT}/experiment_registry.csv"
DATASET_MAP_FILE="${EXPERIMENT_ROOT}/dataset_map.csv"
PARAMETER_FILE="${EXPERIMENT_ROOT}/protocol/davao_catalog_parameter_registry.csv"
RESULTS_DIRECTORY="${EXPERIMENT_ROOT}/results"

AWK_PROGRAM="${PROJECT_ROOT}/scripts/experiments/fm_v1_3_usgs/extract_raw_observables.awk"

OUTPUT_FILE="${RESULTS_DIRECTORY}/raw_catalog_observables.csv"
WINDOW_FILE="${RESULTS_DIRECTORY}/raw_catalog_windows.csv"

: "${CAMEFF_CATALOG_REPO:?Set CAMEFF_CATALOG_REPO to the catalog repository path}"

for command_name in gawk date mkdir; do
    command -v "${command_name}" >/dev/null 2>&1 || {
        printf 'ERROR: Required command not found: %s\n' \
            "${command_name}" >&2
        exit 1
    }
done

for required_file in \
    "${REGISTRY_FILE}" \
    "${DATASET_MAP_FILE}" \
    "${PARAMETER_FILE}" \
    "${AWK_PROGRAM}"
do
    [[ -f "${required_file}" ]] || {
        printf 'ERROR: Required file not found:\n  %s\n' \
            "${required_file}" >&2
        exit 1
    }
done

parameter_value() {
    local parameter_name="$1"

    gawk -F',' \
        -v requested_parameter="${parameter_name}" \
        'NR > 1 && $1 == requested_parameter { print $2; exit }' \
        "${PARAMETER_FILE}"
}

recent_rate_window_days="$(parameter_value recent_rate_window_days)"
background_rate_window_days="$(parameter_value background_rate_window_days)"
magnitude_window_days="$(parameter_value magnitude_window_days)"
migration_window_days="$(parameter_value migration_window_days)"
recent_b_window_days="$(parameter_value recent_b_window_days)"
background_b_window_days="$(parameter_value background_b_window_days)"

for parameter_name in \
    recent_rate_window_days \
    background_rate_window_days \
    magnitude_window_days \
    migration_window_days \
    recent_b_window_days \
    background_b_window_days
do
    parameter_value_result="$(parameter_value "${parameter_name}")"

    [[ -n "${parameter_value_result}" ]] || {
        printf 'ERROR: Missing parameter value: %s\n' \
            "${parameter_name}" >&2
        exit 1
    }
done

mkdir -p "${RESULTS_DIRECTORY}"

declare -A FEATURE_PATHS

while IFS=',' read -r \
    dataset_id \
    feature_relative_path \
    outcome_relative_path
do
    dataset_id="${dataset_id%$'\r'}"
    feature_relative_path="${feature_relative_path%$'\r'}"

    [[ "${dataset_id}" == "catalog_dataset" ]] && continue
    [[ -z "${dataset_id}" ]] && continue

    FEATURE_PATHS["${dataset_id}"]="${feature_relative_path}"
done < "${DATASET_MAP_FILE}"

printf '%s\n' \
'case_id,total_predecision_count,recent_count,recent_rate_per_day,background_count,background_rate_per_day,rate_ratio,recent_magnitude_count,recent_max_magnitude,previous_magnitude_count,previous_max_magnitude,delta_max_magnitude,recent_mean_distance_km,recent_min_distance_km,migration_event_count,migration_distance_slope_km_per_day,recent_b_event_count,recent_b_mean_magnitude,background_b_event_count,background_b_mean_magnitude,postdecision_catalog_rows_excluded' \
> "${OUTPUT_FILE}"

printf '%s\n' \
'case_id,lookback_start_utc,decision_time_utc,recent_rate_start_utc,background_rate_start_utc,magnitude_start_utc,previous_magnitude_start_utc,migration_start_utc,recent_b_start_utc,background_b_start_utc,parameter_set_id,estimator_status' \
> "${WINDOW_FILE}"

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
    minimum_catalog_magnitude \
    target_magnitude \
    forecast_horizon_days \
    analysis_stage \
    declared_math_path \
    existing_outcome_label \
    case_status
do
    case_id="${case_id%$'\r'}"
    catalog_dataset="${catalog_dataset%$'\r'}"
    decision_time_utc="${decision_time_utc%$'\r'}"
    lookback_start_utc="${lookback_start_utc%$'\r'}"
    center_latitude="${center_latitude%$'\r'}"
    center_longitude="${center_longitude%$'\r'}"
    radius_km="${radius_km%$'\r'}"
    minimum_catalog_magnitude="${minimum_catalog_magnitude%$'\r'}"

    [[ "${case_id}" == "case_id" ]] && continue
    [[ -z "${case_id}" ]] && continue

    feature_relative_path="${FEATURE_PATHS[${catalog_dataset}]:-}"

    [[ -n "${feature_relative_path}" ]] || {
        printf 'ERROR: No feature path for dataset: %s\n' \
            "${catalog_dataset}" >&2
        exit 1
    }

    feature_catalog="${CAMEFF_CATALOG_REPO}/${feature_relative_path}"

    [[ -f "${feature_catalog}" ]] || {
        printf 'ERROR: Feature catalog missing:\n  %s\n' \
            "${feature_catalog}" >&2
        exit 1
    }

    decision_epoch="$(date -u --date="${decision_time_utc}" +%s)"
    lookback_start_epoch="$(date -u --date="${lookback_start_utc}" +%s)"

    recent_start_epoch=$((decision_epoch - recent_rate_window_days * 86400))
    background_start_epoch=$((decision_epoch - background_rate_window_days * 86400))

    magnitude_start_epoch=$((decision_epoch - magnitude_window_days * 86400))
    previous_magnitude_start_epoch=$((decision_epoch - 2 * magnitude_window_days * 86400))

    migration_start_epoch=$((decision_epoch - migration_window_days * 86400))

    recent_b_start_epoch=$((decision_epoch - recent_b_window_days * 86400))
    background_b_start_epoch=$((decision_epoch - background_b_window_days * 86400))

    recent_start_utc="$(
        date -u --date="@${recent_start_epoch}" '+%Y-%m-%dT%H:%M:%SZ'
    )"

    background_start_utc="$(
        date -u --date="@${background_start_epoch}" '+%Y-%m-%dT%H:%M:%SZ'
    )"

    magnitude_start_utc="$(
        date -u --date="@${magnitude_start_epoch}" '+%Y-%m-%dT%H:%M:%SZ'
    )"

    previous_magnitude_start_utc="$(
        date -u --date="@${previous_magnitude_start_epoch}" '+%Y-%m-%dT%H:%M:%SZ'
    )"

    migration_start_utc="$(
        date -u --date="@${migration_start_epoch}" '+%Y-%m-%dT%H:%M:%SZ'
    )"

    recent_b_start_utc="$(
        date -u --date="@${recent_b_start_epoch}" '+%Y-%m-%dT%H:%M:%SZ'
    )"

    background_b_start_utc="$(
        date -u --date="@${background_b_start_epoch}" '+%Y-%m-%dT%H:%M:%SZ'
    )"

    TZ=UTC gawk \
        -v case_id="${case_id}" \
        -v decision_epoch="${decision_epoch}" \
        -v lookback_start_epoch="${lookback_start_epoch}" \
        -v recent_start_epoch="${recent_start_epoch}" \
        -v background_start_epoch="${background_start_epoch}" \
        -v magnitude_start_epoch="${magnitude_start_epoch}" \
        -v previous_magnitude_start_epoch="${previous_magnitude_start_epoch}" \
        -v migration_start_epoch="${migration_start_epoch}" \
        -v recent_b_start_epoch="${recent_b_start_epoch}" \
        -v background_b_start_epoch="${background_b_start_epoch}" \
        -v recent_window_days="${recent_rate_window_days}" \
        -v background_window_days="${background_rate_window_days}" \
        -v center_lat="${center_latitude}" \
        -v center_lon="${center_longitude}" \
        -v radius_km="${radius_km}" \
        -v catalog_min_magnitude="${minimum_catalog_magnitude}" \
        -f "${AWK_PROGRAM}" \
        "${feature_catalog}" \
        >> "${OUTPUT_FILE}"

    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
        "${case_id}" \
        "${lookback_start_utc}" \
        "${decision_time_utc}" \
        "${recent_start_utc}" \
        "${background_start_utc}" \
        "${magnitude_start_utc}" \
        "${previous_magnitude_start_utc}" \
        "${migration_start_utc}" \
        "${recent_b_start_utc}" \
        "${background_b_start_utc}" \
        "davao_catalog_v0.1" \
        "provisional_not_frozen" \
        >> "${WINDOW_FILE}"

    printf 'Extracted raw observables: %s\n' "${case_id}"

done < "${REGISTRY_FILE}"

printf '\nRaw observable extraction complete.\n'
printf 'Observables: %s\n' "${OUTPUT_FILE}"
printf 'Windows:     %s\n' "${WINDOW_FILE}"
