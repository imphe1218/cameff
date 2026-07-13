# Cebu 2025 Hindcast Experiment

This experiment evaluates the frozen CAMEFF b1.0.0 validation pipeline
against the 30 September 2025 Offshore Northern Cebu earthquake.

## Target event

* Time: 2025-09-30 13:59:43 UTC
* Latitude: 11.156 degrees north
* Longitude: 124.111 degrees east
* Depth: 10.0 km
* Magnitude: Mw6.9
* USGS event identifier: us6000rdrz
* Tectonic setting: shallow crustal faulting
* Associated fault: Bogo Bay Fault

## Frozen validation rules

The following rules were fixed before evaluating Cebu:

* Warning threshold: 0.73
* Forecast lead time: 24 hours
* Evidence window: 365 days
* Analysis radius: 500 km
* Minimum catalog magnitude: M2.5
* Minimum evidence events: 20

The threshold and CAMEFF core mathematics must not be modified to fit
the Cebu result.

## Why this case matters

Japan and Kamchatka are subduction-zone megathrust cases. Cebu is a
shallower crustal earthquake associated with a locally significant fault
system.

This makes Cebu an important test of whether CAMEFF can generalize beyond
large subduction preparation sequences.

A miss must be recorded as a miss rather than corrected by changing the
frozen threshold after observing the result.

## Validation catalog

The catalog covers approximately five years before the target.

Acquisition parameters:

* Start: 2020-09-29 13:59:43 UTC
* End: 2025-09-30 13:59:42 UTC
* Centre: 11.156, 124.111
* Radius: 500 km
* Minimum magnitude: M2.5
* Event type: earthquake
* Ordering: ascending origin time

The catalog ends one second before the target event, preventing direct
target leakage.

## Download

From the repository root:

```bash
./experiments/cebu_2025/fetch_usgs_raw.sh
```

The raw catalog is written to:

```text
experiments/cebu_2025/data/raw/usgs_cebu_2020_2025_raw.csv
```

## Normalize

Build the CAMEFF utilities:

```bash
cmake --build build
```

Normalize the catalog:

```bash
./experiments/cebu_2025/normalize_catalog.sh
```

The normalized file is written to:

```text
experiments/cebu_2025/data/normalized/catalog.csv
```

## Planned positive window

* Evidence start: 2024-09-29 13:59:43 UTC
* Cutoff: 2025-09-29 13:59:43 UTC
* Target: 2025-09-30 13:59:43 UTC
* Forecast lead time: 24 hours
* Expected label: positive

## Planned controls

* Cebu 2021 annual control
* Cebu 2022 annual control
* Cebu 2023 annual control
* Cebu 2024 annual control

The controls will be classified using the same frozen threshold of 0.73.

## Interpretation

The hazard-evidence output is an uncalibrated score. It is not an
earthquake probability.

This experiment is an untouched retrospective generalization test.

## Run the positive hindcast

From the repository root:

```bash
./experiments/cebu_2025/run_hindcast.sh
```

The experiment uses a cutoff exactly twenty-four hours before the target
earthquake.

The generated report is written to:

```text
experiments/cebu_2025/results/cebu_2025_hindcast.txt
```

## Run the validation suite

Run the four annual controls and the positive target window:

```bash
./experiments/cebu_2025/run_validation_suite.sh
```

Inspect the results:

```bash
column -s, -t \
  experiments/cebu_2025/results/cebu_validation_windows.csv
```

The `predicted_label` field uses the frozen warning threshold of 0.73:

* `1` means CAMEFF issued a warning.
* `0` means the hazard score remained below the threshold.

## Frozen-threshold metrics

Generate the Cebu metrics:

```bash
./experiments/cebu_2025/run_threshold_report.sh
```

Inspect the report:

```bash
column -s, -t \
  experiments/cebu_2025/results/cebu_frozen_threshold_metrics.csv
```

The warning threshold must not be modified based on the Cebu outcome.
A result below 0.73 for the positive target window must be recorded as a
miss.
