#!/usr/bin/env bash

set -euo pipefail

PROJECT_ROOT="$(
    cd "$(dirname "${BASH_SOURCE[0]}")/../../.." &&
    pwd
)"

EXPERIMENT_ROOT="${PROJECT_ROOT}/experiments/fm_v1_3_wider_usgs_hindcast"
REGISTRY_FILE="${EXPERIMENT_ROOT}/experiment_registry.csv"
DATASET_MAP_FILE="${EXPERIMENT_ROOT}/dataset_map.csv"

: "${CAMEFF_CATALOG_REPO:?Set CAMEFF_CATALOG_REPO to the catalog repository path}"

[[ -f "${REGISTRY_FILE}" ]] || {
    printf 'ERROR: Registry not found:\n  %s\n' \
        "${REGISTRY_FILE}" >&2
    exit 1
}

[[ -f "${DATASET_MAP_FILE}" ]] || {
    printf 'ERROR: Dataset map not found:\n  %s\n' \
        "${DATASET_MAP_FILE}" >&2
    exit 1
}

[[ -d "${CAMEFF_CATALOG_REPO}" ]] || {
    printf 'ERROR: Catalog repository not found:\n  %s\n' \
        "${CAMEFF_CATALOG_REPO}" >&2
    exit 1
}

declare -A FEATURE_PATHS
declare -A OUTCOME_PATHS

while IFS=',' read -r \
    dataset_id \
    feature_relative_path \
    outcome_relative_path
do
    dataset_id="${dataset_id%$'\r'}"
    feature_relative_path="${feature_relative_path%$'\r'}"
    outcome_relative_path="${outcome_relative_path%$'\r'}"

    [[ "${dataset_id}" == "catalog_dataset" ]] && continue
    [[ -z "${dataset_id}" ]] && continue

    FEATURE_PATHS["${dataset_id}"]="${feature_relative_path}"
    OUTCOME_PATHS["${dataset_id}"]="${outcome_relative_path}"
done < "${DATASET_MAP_FILE}"

errors=0

printf 'Validating registry datasets...\n\n'

while IFS=',' read -r \
    case_id \
    case_role \
    region_name \
    tectonic_regime \
    catalog_dataset \
    remaining_fields
do
    case_id="${case_id%$'\r'}"
    catalog_dataset="${catalog_dataset%$'\r'}"

    [[ "${case_id}" == "case_id" ]] && continue
    [[ -z "${case_id}" ]] && continue

    if [[ -z "${FEATURE_PATHS[${catalog_dataset}]+x}" ]]; then
        printf 'ERROR: %-25s no dataset-map entry for %s\n' \
            "${case_id}" \
            "${catalog_dataset}" >&2
        errors=$((errors + 1))
        continue
    fi

    feature_file="${CAMEFF_CATALOG_REPO}/${FEATURE_PATHS[${catalog_dataset}]}"
    outcome_file="${CAMEFF_CATALOG_REPO}/${OUTCOME_PATHS[${catalog_dataset}]}"

    case_errors=0

    if [[ ! -f "${feature_file}" ]]; then
        printf 'ERROR: Feature catalog missing for %s:\n  %s\n' \
            "${case_id}" \
            "${feature_file}" >&2
        errors=$((errors + 1))
        case_errors=$((case_errors + 1))
    fi

    if [[ ! -f "${outcome_file}" ]]; then
        printf 'ERROR: Outcome catalog missing for %s:\n  %s\n' \
            "${case_id}" \
            "${outcome_file}" >&2
        errors=$((errors + 1))
        case_errors=$((case_errors + 1))
    fi

    if (( case_errors == 0 )); then
        printf 'OK: %-25s dataset=%s\n' \
            "${case_id}" \
            "${catalog_dataset}"
    fi
done < "${REGISTRY_FILE}"

printf '\n'

if (( errors > 0 )); then
    printf 'Catalog validation failed with %d error(s).\n' \
        "${errors}" >&2
    exit 1
fi

printf 'Catalog validation: OK\n'
