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

RAW_CATALOG="${SCRIPT_DIR}/data/raw/usgs_venezuela_2021_2026_raw.csv"

NORMALIZED_DIRECTORY="${SCRIPT_DIR}/data/normalized"
NORMALIZED_CATALOG="${NORMALIZED_DIRECTORY}/catalog.csv"

NORMALIZER="${REPOSITORY_ROOT}/build/cameff_catalog_normalizer"

if [[ ! -x "${NORMALIZER}" ]]; then
    echo "Catalog normalizer not found:" >&2
    echo "  ${NORMALIZER}" >&2
    echo "Build the project first:" >&2
    echo "  cmake --build build" >&2
    exit 1
fi

if [[ ! -f "${RAW_CATALOG}" ]]; then
    echo "Raw Venezuela catalog not found:" >&2
    echo "  ${RAW_CATALOG}" >&2
    echo "Run the acquisition script first." >&2
    exit 1
fi

mkdir -p "${NORMALIZED_DIRECTORY}"

"${NORMALIZER}" \
    "${RAW_CATALOG}" \
    "${NORMALIZED_CATALOG}"

echo "Normalized catalog created:"
echo "  ${NORMALIZED_CATALOG}"