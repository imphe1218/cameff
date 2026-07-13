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

RESULT_DIRECTORY="${SCRIPT_DIR}/results"

NORMALIZED_WINDOWS_FILE="${RESULT_DIRECTORY}/cameff_b1_milestone_3_windows.csv"
CASE_SUMMARY_FILE="${RESULT_DIRECTORY}/cameff_b1_milestone_3_cases.csv"
METRICS_FILE="${RESULT_DIRECTORY}/cameff_b1_milestone_3_metrics.csv"
REPORT_FILE="${RESULT_DIRECTORY}/cameff_b1_milestone_3_report.txt"

JAPAN_FILE="${REPOSITORY_ROOT}/experiments/japan_2011/results/japan_validation_windows.csv"
RUSSIA_FILE="${REPOSITORY_ROOT}/experiments/russia_2025/results/russia_validation_windows.csv"
CEBU_FILE="${REPOSITORY_ROOT}/experiments/cebu_2025/results/cebu_validation_windows.csv"
DAVAO_FILE="${REPOSITORY_ROOT}/experiments/davao_2026/results/davao_validation_windows.csv"
VENEZUELA_FILE="${REPOSITORY_ROOT}/experiments/venezuela_2026/results/venezuela_validation_windows.csv"

FROZEN_THRESHOLD="0.73"
FORECAST_LEAD_HOURS="24"
EVIDENCE_WINDOW_DAYS="365"
ANALYSIS_RADIUS_KM="500"
MINIMUM_MAGNITUDE="2.5"
MINIMUM_EVENTS="20"

mkdir -p "${RESULT_DIRECTORY}"

require_file()
{
    local file="$1"

    if [[ ! -f "${file}" ]]; then
        echo "Required validation file not found:" >&2
        echo "  ${file}" >&2
        exit 1
    fi

    if [[ ! -s "${file}" ]]; then
        echo "Required validation file is empty:" >&2
        echo "  ${file}" >&2
        exit 1
    fi
}

require_file "${JAPAN_FILE}"
require_file "${RUSSIA_FILE}"
require_file "${CEBU_FILE}"
require_file "${DAVAO_FILE}"
require_file "${VENEZUELA_FILE}"

cat > "${NORMALIZED_WINDOWS_FILE}" <<'EOF'
case_id,region,label,predicted_label,evaluable,observation_start,cutoff,evidence_event_count,hazard_evidence,fused_confidence,dominant_pattern,dominant_expert,outcome
EOF

append_validation_file()
{
    local source_file="$1"
    local region="$2"

    awk \
        -F, \
        -v OFS=',' \
        -v region="${region}" \
        -v threshold="${FROZEN_THRESHOLD}" '
        
        FNR == 1 {
            delete column
            delete required_columns

            for (i = 1; i <= NF; i++) {
                column[$i] = i
            }

            required_columns[1] = "case_id"
            required_columns[2] = "label"
            required_columns[3] = "evidence_event_count"
            required_columns[4] = "hazard_evidence"
            required_columns[5] = "fused_confidence"
            required_columns[6] = "dominant_pattern"
            required_columns[7] = "dominant_expert"

            for (i = 1; i <= 7; i++) {
                name = required_columns[i]

                if (!(name in column)) {
                    printf "Missing required column %s in %s\n", \
                        name, FILENAME > "/dev/stderr"
                    exit 2
                }
            }

            next
        }

        {
            case_id = $(column["case_id"])
            label = $(column["label"])
            event_count = $(column["evidence_event_count"])
            hazard = $(column["hazard_evidence"])
            confidence = $(column["fused_confidence"])
            pattern = $(column["dominant_pattern"])
            expert = $(column["dominant_expert"])

            observation_start = \
                ("observation_start" in column) \
                ? $(column["observation_start"]) \
                : "NA"

            cutoff = \
                ("cutoff" in column) \
                ? $(column["cutoff"]) \
                : "NA"

            if ("predicted_label" in column) {
                prediction = $(column["predicted_label"])
            } else if (hazard == "NA" || hazard == "") {
                prediction = "NA"
            } else if ((hazard + 0.0) >= (threshold + 0.0)) {
                prediction = "1"
            } else {
                prediction = "0"
            }

            evaluable = (prediction == "NA") ? "0" : "1"

            if (prediction == "NA") {
                outcome = "NOT_EVALUATED"
            } else if (label == "1" && prediction == "1") {
                outcome = "HIT"
            } else if (label == "1" && prediction == "0") {
                outcome = "MISS"
            } else if (label == "0" && prediction == "1") {
                outcome = "FALSE_POSITIVE"
            } else {
                outcome = "TRUE_NEGATIVE"
            }

            print \
                case_id, \
                region, \
                label, \
                prediction, \
                evaluable, \
                observation_start, \
                cutoff, \
                event_count, \
                hazard, \
                confidence, \
                pattern, \
                expert, \
                outcome
        }
    ' "${source_file}" >> "${NORMALIZED_WINDOWS_FILE}"
}

append_validation_file "${JAPAN_FILE}" "Japan"
append_validation_file "${RUSSIA_FILE}" "Russia"
append_validation_file "${CEBU_FILE}" "Cebu"
append_validation_file "${DAVAO_FILE}" "Davao"
append_validation_file "${VENEZUELA_FILE}" "Venezuela"

cat > "${CASE_SUMMARY_FILE}" <<'EOF'
case_id,region,label,predicted_label,evaluable,evidence_event_count,hazard_evidence,fused_confidence,dominant_pattern,dominant_expert,outcome
EOF

awk -F, -v OFS=',' '
    NR == 1 {
        next
    }

    $3 == "1" {
        print \
            $1, \
            $2, \
            $3, \
            $4, \
            $5, \
            $8, \
            $9, \
            $10, \
            $11, \
            $12, \
            $13
    }
' "${NORMALIZED_WINDOWS_FILE}" >> "${CASE_SUMMARY_FILE}"

awk \
    -F, \
    -v threshold="${FROZEN_THRESHOLD}" '
    NR == 1 {
        next
    }

    {
        label = $3
        prediction = $4

        if (prediction == "NA") {
            not_evaluable++
            next
        }

        if (label == "1" && prediction == "1") {
            tp++
        } else if (label == "0" && prediction == "1") {
            fp++
        } else if (label == "0" && prediction == "0") {
            tn++
        } else if (label == "1" && prediction == "0") {
            fn++
        }
    }

    END {
        if ((tp + fp) > 0) {
            precision = sprintf("%.6f", tp / (tp + fp))
        } else {
            precision = "NA"
        }

        if ((tp + fn) > 0) {
            recall = sprintf("%.6f", tp / (tp + fn))
        } else {
            recall = "NA"
        }

        if ((tn + fp) > 0) {
            specificity = sprintf("%.6f", tn / (tn + fp))
            false_positive_rate = sprintf("%.6f", fp / (fp + tn))
        } else {
            specificity = "NA"
            false_positive_rate = "NA"
        }

        if (precision != "NA" && recall != "NA") {
            precision_value = precision + 0.0
            recall_value = recall + 0.0

            if ((precision_value + recall_value) > 0.0) {
                f1 = sprintf("%.6f", 2.0 * precision_value * recall_value / (precision_value + recall_value))
            } else {
                f1 = "0.000000"
            }
        } else {
            f1 = "NA"
        }

        if (recall != "NA" && specificity != "NA") {
            balanced_accuracy = sprintf("%.6f", ((recall + 0.0) + (specificity + 0.0)) / 2.0)
        } else {
            balanced_accuracy = "NA"
        }

        print "threshold,tp,fp,tn,fn,not_evaluable,precision,recall,specificity,false_positive_rate,f1,balanced_accuracy"

        printf "%s,%d,%d,%d,%d,%d,%s,%s,%s,%s,%s,%s\n", \
            threshold, tp, fp, tn, fn, not_evaluable, \
            precision, recall, specificity, false_positive_rate, \
            f1, balanced_accuracy
    }
' "${NORMALIZED_WINDOWS_FILE}" > "${METRICS_FILE}"

METRICS_ROW="$(
    awk -F, '
        NR == 2 {
            print
            exit
        }
    ' "${METRICS_FILE}"
)"

if [[ -z "${METRICS_ROW}" ]]; then
    echo "Aggregate metrics row was not generated." >&2
    exit 1
fi

IFS=',' read -r \
    METRIC_THRESHOLD \
    METRIC_TP \
    METRIC_FP \
    METRIC_TN \
    METRIC_FN \
    METRIC_NOT_EVALUABLE \
    METRIC_PRECISION \
    METRIC_RECALL \
    METRIC_SPECIFICITY \
    METRIC_FALSE_POSITIVE_RATE \
    METRIC_F1 \
    METRIC_BALANCED_ACCURACY \
    <<< "${METRICS_ROW}"

{
    echo "CAMEFF b1.0.0 Milestone 3 Consolidated Validation Report"
    echo "======================================================="
    echo
    echo "Frozen validation configuration"
    echo "-------------------------------"
    echo "Warning threshold: ${FROZEN_THRESHOLD}"
    echo "Forecast lead time: ${FORECAST_LEAD_HOURS} hours"
    echo "Evidence window: ${EVIDENCE_WINDOW_DAYS} days"
    echo "Analysis radius: ${ANALYSIS_RADIUS_KM} km"
    echo "Minimum catalog magnitude: M${MINIMUM_MAGNITUDE}"
    echo "Minimum evidence events: ${MINIMUM_EVENTS}"
    echo
    echo "Positive-case outcomes"
    echo "----------------------"

    awk -F, '
        NR == 1 {
            next
        }

        {
            printf "%-26s region=%-11s score=%-10s confidence=%-10s outcome=%s\n", \
                $1, $2, $7, $8, $11
        }
    ' "${CASE_SUMMARY_FILE}"

    echo
    echo "Aggregate confusion counts"
    echo "--------------------------"
    echo "True positives: ${METRIC_TP}"
    echo "False positives: ${METRIC_FP}"
    echo "True negatives: ${METRIC_TN}"
    echo "False negatives: ${METRIC_FN}"
    echo "Not-evaluable windows: ${METRIC_NOT_EVALUABLE}"
    echo
    echo "Aggregate performance metrics"
    echo "-----------------------------"
    echo "Precision: ${METRIC_PRECISION}"
    echo "Recall: ${METRIC_RECALL}"
    echo "Specificity: ${METRIC_SPECIFICITY}"
    echo "False-positive rate: ${METRIC_FALSE_POSITIVE_RATE}"
    echo "F1 score: ${METRIC_F1}"
    echo "Balanced accuracy: ${METRIC_BALANCED_ACCURACY}"
    echo
    echo "Scientific interpretation"
    echo "-------------------------"
    echo "CAMEFF detected the Japan 2011 and Russia 2025 positive cases."
    echo "CAMEFF missed the Cebu 2025, Davao 2026, and Venezuela 2026"
    echo "positive cases under the frozen threshold."
    echo
    echo "No false positives occurred among the sixteen evaluable"
    echo "negative-control windows."
    echo
    echo "Four Venezuela control windows were not evaluable because"
    echo "their evidence-event counts were below the frozen minimum"
    echo "of ${MINIMUM_EVENTS} events."
    echo
    echo "All five positive cases selected SUBDUCTION_PREPARATION as"
    echo "the dominant pattern and baseline as the dominant expert."
    echo "This indicates insufficient tectonic-regime discrimination"
    echo "and insufficient expert specialization in the current baseline."
    echo
    echo "The hazard-evidence value is an uncalibrated evidence score."
    echo "It must not be interpreted as an earthquake-occurrence"
    echo "probability."
    echo
    echo "Milestone 3 preserves these results as the frozen CAMEFF"
    echo "b1.0.0 validation baseline. Architectural changes intended"
    echo "to address the misses belong to Milestone 4."
} > "${REPORT_FILE}"

echo "Milestone 3 aggregate report complete."
echo
echo "Normalized validation windows:"
echo "  ${NORMALIZED_WINDOWS_FILE}"
echo
echo "Case summary:"
echo "  ${CASE_SUMMARY_FILE}"
echo
echo "Aggregate metrics:"
echo "  ${METRICS_FILE}"
echo
echo "Human-readable report:"
echo "  ${REPORT_FILE}"