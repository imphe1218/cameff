# CAMEFF b1.0.0 — Milestone 1

Pure C17 foundation for the **Classification-Adaptive Multi-Expert Forecasting Framework**.

Milestone 1 deliberately implements the framework substrate, not a claimed operational earthquake predictor:

- time-causal CSV catalog ingestion;
- geographic and temporal event selection;
- eight normalized, uncertainty-aware signal channels;
- explicit fault-map-confidence input;
- deterministic evidence aggregation;
- `BACKGROUND`, `WATCH`, `ELEVATED`, and `INSUFFICIENT_DATA` states;
- external configuration, CLI, unit tests, CMake/Ninja build.

The weights and thresholds are transparent engineering defaults. They are not calibrated probabilities and must not be treated as public-warning criteria.

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run

```bash
./build/cameff analyze \
  --catalog data/examples/catalog.csv \
  --cutoff 2026-01-31T00:00:00Z \
  --lat 10.0 --lon 123.0 --radius-km 100 \
  --lookback-days 7 --config config/default.conf
```

## Milestone boundary

Included: data contracts, signal extraction, uncertainty propagation, evidence-level output, portability tests.

Deferred: tectonic-pattern classifier, expert modules, expert gating, calibration, hindcast runner, USGS/FDSN adapters, negative controls, probability calibration, persistence, evolutionary optimization, and operational hazard mapping.

---

# CAMEFF b1.0.0 — Milestone 2: Expert Framework Architecture

Status: Complete

Milestone 2 evolved the reference implementation into an expert-driven earthquake analysis framework.

## Implemented components:

### Pattern Classification

Added deterministic soft pattern classification:

Background seismic behavior
Subduction preparation
Crustal fault activation
Seismic cascade activity
Volcanic unrest
Transform fault activity
Unknown / insufficient information

The classifier produces normalized pattern memberships:

[
\sum_i p_i = 1
]

where each (p_i) represents relative compatibility with a seismic process pattern.

### Expert Registry and Routing

Added an expert orchestration layer:

Expert registration
Applicability evaluation
Pattern-based routing
Routing strength calculation
Explicit expert abstention

Routing strength:

[
g_e = a_e \times p_e
]

where:

(a_e) = expert applicability
(p_e) = pattern membership

Experts can abstain when their specialization is not supported.

### Specialized Experts

Implemented initial expert modules:

Baseline expert
Preserves Milestone 1 behavior
Subduction expert
Handles subduction-related preparation patterns
Crustal fault expert
Handles crustal activation and hidden-fault uncertainty scenarios
Seismic cascade expert
Handles clustered and cascading seismic activity

### Confidence-Aware Fusion

Added multi-expert evidence fusion.

Active experts contribute according to:

[
\omega_e = g_e c_e
]

where:

(g_e) = routing strength
(c_e) = expert confidence

Fused evidence:

[
H =
\frac{\sum_e \omega_e H_e}
{\sum_e \omega_e}
]

Abstaining experts are excluded from fusion while remaining visible in explanations.

### End-to-End Evaluation Pipeline

Added complete processing flow:

Signal Frame
      |
      v
Pattern Classification
      |
      v
Expert Registry
      |
      v
Expert Routing
      |
      v
Expert Evaluation
      |
      v
Confidence Fusion
      |
      v
Explainable Assessment

## Structural Hindcast Scenarios

Added architecture validation scenarios:

Japan subduction scenario
Russia subduction scenario
Cebu crustal fault scenario
Davao mixed tectonic scenario
Venezuela transform scenario

These tests validate architectural behavior and routing logic.

They are not claims of operational earthquake prediction accuracy.

## Current Capability After Milestone 2

CAMEFF b1.0.0 currently provides:

Deterministic earthquake process classification
Expert-based interpretation architecture
Confidence-aware reasoning
Explainable assessment output
Extensible expert framework

Future milestones will focus on:

Additional expert models
Real catalog integration
Historical hindcast experiments
Calibration against observed earthquake catalogs
Operational hazard visualization
Scientific validation

## Milestone 3 — Historical Hindcast and Frozen Validation

Milestone 3 establishes the first reproducible historical-validation
baseline for CAMEFF b1.0.0.

The core signal mathematics, expert-fusion logic, warning threshold, and
validation configuration were frozen before the final test cases were
evaluated. Results were preserved even when the framework missed the
target event.

### Frozen validation configuration

* Warning threshold: `0.73`
* Forecast lead time: `24 hours`
* Evidence window: `365 days`
* Analysis radius: `500 km`
* Minimum catalog magnitude: `M2.5`
* Minimum evidence events: `20`

The CAMEFF hazard-evidence output is an uncalibrated evidence score. It
must not be interpreted as an earthquake-occurrence probability.

### Historical positive cases

| Case           | Evidence events | Hazard evidence | Fused confidence | Outcome |
| -------------- | --------------: | --------------: | ---------------: | ------- |
| Japan 2011     |             235 |        0.780870 |         0.953963 | HIT     |
| Russia 2025    |             626 |        0.850781 |         0.910297 | HIT     |
| Cebu 2025      |             236 |        0.602291 |         0.841350 | MISS    |
| Davao 2026     |             906 |        0.685162 |         0.872197 | MISS    |
| Venezuela 2026 |              33 |        0.514422 |         0.954718 | MISS    |

### Aggregate frozen-validation results

The consolidated validation set contains:

* 5 evaluable positive windows
* 16 evaluable negative-control windows
* 4 non-evaluable Venezuela control windows

The aggregate confusion counts are:

| Metric                | Count |
| --------------------- | ----: |
| True positives        |     2 |
| False positives       |     0 |
| True negatives        |    16 |
| False negatives       |     3 |
| Non-evaluable windows |     4 |

The aggregate performance metrics are:

| Metric              |    Value |
| ------------------- | -------: |
| Precision           | 1.000000 |
| Recall              | 0.400000 |
| Specificity         | 1.000000 |
| False-positive rate | 0.000000 |
| F1 score            | 0.571429 |
| Balanced accuracy   | 0.700000 |

### Interpretation

CAMEFF detected the Japan 2011 and Russia 2025 positive cases while
remaining below the frozen warning threshold for all sixteen evaluable
negative controls.

The framework missed the Cebu 2025, Davao 2026, and Venezuela 2026
positive cases.

All five positive cases were routed to:

```text
SUBDUCTION_PREPARATION
```

and all selected:

```text
baseline
```

as the dominant expert.

This indicates that the current architecture does not yet provide
sufficient tectonic-regime discrimination or expert specialization.
Shallow crustal, strike-slip, regional subduction, and sparse-catalog
conditions are not being routed distinctly enough.

The fault-map-confidence signal also remains a fixed placeholder:

```text
fault_map_confidence=0.500000
supporting_events=0
```

High fused confidence was observed for both correct and incorrect
classifications. Fused confidence therefore represents confidence in the
available evidence frame, not confidence that a target earthquake will
occur.

### Reproduce the consolidated report

Run:

```bash
./experiments/milestone_3/run_aggregate_report.sh
```

Generated outputs:

```text
experiments/milestone_3/results/cameff_b1_milestone_3_windows.csv
experiments/milestone_3/results/cameff_b1_milestone_3_cases.csv
experiments/milestone_3/results/cameff_b1_milestone_3_metrics.csv
experiments/milestone_3/results/cameff_b1_milestone_3_report.txt
```

### Milestone conclusion

Milestone 3 is the official frozen validation baseline for CAMEFF
b1.0.0.

The misses are not corrected retroactively. They are preserved as
evidence of the limitations of the current architecture.

Milestone 4 will address:

* tectonic-regime routing;
* specialized experts;
* shallow-crustal and strike-slip patterns;
* fault-aware spatial geometry;
* multi-scale analysis regions;
* sparse-catalog handling;
* preservation of the Japan and Russia hits;
* independent holdout validation.
