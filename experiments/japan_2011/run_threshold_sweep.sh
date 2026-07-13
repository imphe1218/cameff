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

VALIDATION_FILE="${SCRIPT_DIR}/results/japan_validation_windows.csv"
OUTPUT_FILE="${SCRIPT_DIR}/results/japan_threshold_sweep.csv"
SWEEP_EXECUTABLE="${REPOSITORY_ROOT}/build/cameff_threshold_sweep"

if [[ ! -x "${SWEEP_EXECUTABLE}" ]]; then
    echo "Threshold sweep executable not found:" >&2
    echo "  ${SWEEP_EXECUTABLE}" >&2
    exit 1
fi

if [[ ! -f "${VALIDATION_FILE}" ]]; then
    echo "Validation results not found:" >&2
    echo "  ${VALIDATION_FILE}" >&2
    echo "Run the validation suite first." >&2
    exit 1
fi

"${SWEEP_EXECUTABLE}" \
    "${VALIDATION_FILE}" \
    | tee "${OUTPUT_FILE}"

echo
echo "Threshold sweep written to:"
echo "  ${OUTPUT_FILE}"