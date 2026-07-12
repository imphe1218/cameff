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
