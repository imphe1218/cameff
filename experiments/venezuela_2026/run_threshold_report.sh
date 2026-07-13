#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(
    cd "$(dirname "${BASH_SOURCE[0]}")"
    pwd
)"

INPUT_FILE="${SCRIPT_DIR}/results/venezuela_validation_windows.csv"
OUTPUT_FILE="${SCRIPT_DIR}/results/venezuela_frozen_threshold_metrics.csv"

if [[ ! -f "${INPUT_FILE}" ]]; then
    echo "Venezuela validation results not found:" >&2
    echo "  ${INPUT_FILE}" >&2
    echo "Run the validation suite first." >&2
    exit 1
fi

awk -F, '
    NR == 1 {
        next
    }

   {
    if ($3 == "NA") {
        not_evaluable++
        next
    }

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
        precision = (tp + fp) ? sprintf("%.6f", tp / (tp + fp)) : "NA"
        recall = (tp + fn) ? sprintf("%.6f", tp / (tp + fn)) : "NA"
        specificity = (tn + fp) ? sprintf("%.6f", tn / (tn + fp)) : "NA"
        fpr = (fp + tn) ? sprintf("%.6f", fp / (fp + tn)) : "NA"

        if (precision != "NA" && recall != "NA" &&
            ((precision + 0.0) + (recall + 0.0)) > 0.0) {
            f1 = sprintf("%.6f", 2.0 * (precision + 0.0) * (recall + 0.0) / ((precision + 0.0) + (recall + 0.0)))
        } else if (precision != "NA" && recall != "NA") {
            f1 = "0.000000"
        } else {
            f1 = "NA"
        }

        if (recall != "NA" && specificity != "NA") {
            balanced_accuracy = sprintf("%.6f", ((recall + 0.0) + (specificity + 0.0)) / 2.0)
        } else {
            balanced_accuracy = "NA"
        }

        print "threshold,tp,fp,tn,fn,not_evaluable,precision,recall,specificity,false_positive_rate,f1,balanced_accuracy"

        printf "0.73,%d,%d,%d,%d,%d,%s,%s,%s,%s,%s,%s\n", \
            tp, fp, tn, fn, not_evaluable, \
            precision, recall, specificity, fpr, f1, balanced_accuracy
    }
' "${INPUT_FILE}" | tee "${OUTPUT_FILE}"

echo
echo "Frozen-threshold report written to:"
echo "  ${OUTPUT_FILE}"