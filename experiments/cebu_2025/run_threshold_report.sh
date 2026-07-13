#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(
    cd "$(dirname "${BASH_SOURCE[0]}")"
    pwd
)"

INPUT_FILE="${SCRIPT_DIR}/results/cebu_validation_windows.csv"
OUTPUT_FILE="${SCRIPT_DIR}/results/cebu_frozen_threshold_metrics.csv"

if [[ ! -f "${INPUT_FILE}" ]]; then
    echo "Cebu validation results not found:" >&2
    echo "  ${INPUT_FILE}" >&2
    echo "Run the validation suite first." >&2
    exit 1
fi

awk -F, '
    NR == 1 {
        next
    }

    {
        label = $2 + 0
        prediction = $3 + 0

        if (label == 1 && prediction == 1) {
            tp++
        } else if (label == 0 && prediction == 1) {
            fp++
        } else if (label == 0 && prediction == 0) {
            tn++
        } else if (label == 1 && prediction == 0) {
            fn++
        }
    }

    END {
        precision = (tp + fp) ? tp / (tp + fp) : 0
        recall = (tp + fn) ? tp / (tp + fn) : 0
        specificity = (tn + fp) ? tn / (tn + fp) : 0
        fpr = (fp + tn) ? fp / (fp + tn) : 0

        f1 = (precision + recall) \
            ? 2 * precision * recall / (precision + recall) \
            : 0

        balanced_accuracy = (recall + specificity) / 2

        print "threshold,tp,fp,tn,fn,precision,recall,specificity,false_positive_rate,f1,balanced_accuracy"

        printf "0.73,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n", \
            tp, fp, tn, fn, \
            precision, recall, specificity, fpr, f1, balanced_accuracy
    }
' "${INPUT_FILE}" | tee "${OUTPUT_FILE}"

echo
echo "Frozen-threshold report written to:"
echo "  ${OUTPUT_FILE}"