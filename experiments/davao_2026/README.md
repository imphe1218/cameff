# Davao and Southern Mindanao 2026 Hindcast Experiment

This experiment evaluates the frozen CAMEFF b1.0.0 validation pipeline
against the 7 June 2026 M7.8 southern Mindanao earthquake.

## Target event

* Time: 2026-06-07 23:37:41 UTC
* Latitude: 5.599 degrees north
* Longitude: 125.056 degrees east
* Depth: 57.0 km
* Magnitude: M7.8
* USGS event identifier: us7000srb1
* USGS description: 25 km southwest of Kablalan, Philippines
* Regional setting: southern Mindanao and Offshore Sarangani
* Tectonic setting: regional subduction-related tectonic earthquake

## Frozen validation rules

The following rules were fixed before evaluating this case:

* Warning threshold: 0.73
* Forecast lead time: 24 hours
* Evidence window: 365 days
* Analysis radius: 500 km
* Minimum catalog magnitude: M2.5
* Minimum evidence events: 20

Neither the threshold nor the core CAMEFF mathematics may be changed to
fit the Davao result.

## Why this case matters

The event occurred south of Mindanao and affected the wider southern
Mindanao and Davao region.

It differs from the shallow Cebu crustal event in source depth and
regional tectonic structure. It therefore provides another untouched
test of CAMEFF pattern classification and expert fusion.

A positive score below 0.73 must be recorded as a miss.

## Validation catalog

The catalog covers approximately five years before the target event.

Acquisition parameters:

* Start: 2021-06-06 23:37:41 UTC
* End: 2026-06-07 23:37:40 UTC
* Centre: 5.599, 125.056
* Radius: 500 km
* Minimum magnitude: M2.5
* Event type: earthquake
* Ordering: ascending origin time

The catalog ends one second before the target event, preventing direct
target leakage.

## Download

From the repository root:

```bash
./experiments/davao_2026/fetch_usgs_raw.sh
```

The raw catalog is written to:

```text
experiments/davao_2026/data/raw/usgs_davao_2021_2026_raw.csv
```

## Normalize

Build the CAMEFF utilities:

```bash
cmake --build build
```

Normalize the catalog:

```bash
./experiments/davao_2026/normalize_catalog.sh
```

The normalized catalog is written to:

```text
experiments/davao_2026/data/normalized/catalog.csv
```

## Planned positive window

* Evidence start: 2025-06-06 23:37:41 UTC
* Cutoff: 2026-06-06 23:37:41 UTC
* Target: 2026-06-07 23:37:41 UTC
* Forecast lead time: 24 hours
* Expected label: positive

## Planned controls

* Davao 2022 annual control
* Davao 2023 annual control
* Davao 2024 annual control
* Davao 2025 annual control

All controls will use the frozen warning threshold of 0.73.

## Interpretation

CAMEFF hazard evidence is an uncalibrated evidence score and not an
earthquake occurrence probability.

This is an untouched retrospective generalization test.

## Run the positive hindcast

From the repository root:

```bash
./experiments/davao_2026/run_hindcast.sh
```

The positive experiment uses a cutoff exactly twenty-four hours before
the M7.8 target event.

The generated report is written to:

```text
experiments/davao_2026/results/davao_2026_hindcast.txt
```

## Run the validation suite

Run the four annual controls and positive target window:

```bash
./experiments/davao_2026/run_validation_suite.sh
```

Inspect the results:

```bash
column -s, -t \
  experiments/davao_2026/results/davao_validation_windows.csv
```

The `predicted_label` field uses the frozen threshold of 0.73:

* `1` means CAMEFF issued a warning.
* `0` means the score remained below the warning threshold.

## Frozen-threshold metrics

Generate the Davao metrics:

```bash
./experiments/davao_2026/run_threshold_report.sh
```

Inspect the metrics:

```bash
column -s, -t \
  experiments/davao_2026/results/davao_frozen_threshold_metrics.csv
```

The threshold must remain fixed at 0.73. A positive score below this
threshold must be recorded as a miss.
