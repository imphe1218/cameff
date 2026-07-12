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
