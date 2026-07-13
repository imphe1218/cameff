# Venezuela 2026 Hindcast Experiment

This experiment evaluates the frozen CAMEFF b1.0.0 validation pipeline
against the larger earthquake in the 24 June 2026 northern Venezuela
earthquake doublet.

## Target event

* Time: 2026-06-24 22:05:11 UTC
* Latitude: 10.401 degrees north
* Longitude: 68.321 degrees west
* Depth: 10.0 km
* Magnitude: M7.5
* USGS event identifier: us6000t7zp
* Regional setting: northern Venezuela
* Tectonic setting: shallow strike-slip faulting near a plate boundary

## Doublet handling

A separate M7.1 earthquake occurred approximately thirty-nine seconds
before the M7.5 target.

The frozen CAMEFF forecast cutoff is twenty-four hours before the target:

```text
2026-06-23 22:05:11 UTC
```

Consequently, neither the M7.1 event nor the M7.5 target can enter the
evidence window.

The raw acquisition may contain the earlier M7.1 event because the raw
catalog ends one second before the M7.5 target. The hindcast filter must
remove every event at or after the frozen cutoff before signal extraction.

## Frozen validation rules

The following rules were fixed before evaluating Venezuela:

* Warning threshold: 0.73
* Forecast lead time: 24 hours
* Evidence window: 365 days
* Analysis radius: 500 km
* Minimum catalog magnitude: M2.5
* Minimum evidence events: 20

Neither the threshold nor the CAMEFF core mathematics may be modified to
fit the Venezuela outcome.

## Why this case matters

Japan and Kamchatka were major subduction-zone hits.

Cebu and Davao were misses under the frozen threshold.

Venezuela is a shallow strike-slip doublet in a different tectonic
environment. It is therefore an important final untouched test of the
current CAMEFF architecture.

A positive score below 0.73 must be recorded as a miss.

## Validation catalog

The catalog covers approximately five years before the target event.

Acquisition parameters:

* Start: 2021-06-23 22:05:11 UTC
* End: 2026-06-24 22:05:10 UTC
* Centre: 10.401, -68.321
* Radius: 500 km
* Minimum magnitude: M2.5
* Event type: earthquake
* Ordering: ascending origin time

## Download

From the repository root:

```bash
./experiments/venezuela_2026/fetch_usgs_raw.sh
```

The raw catalog is written to:

```text
experiments/venezuela_2026/data/raw/usgs_venezuela_2021_2026_raw.csv
```

## Normalize

Build the CAMEFF utilities:

```bash
cmake --build build
```

Normalize the catalog:

```bash
./experiments/venezuela_2026/normalize_catalog.sh
```

The normalized catalog is written to:

```text
experiments/venezuela_2026/data/normalized/catalog.csv
```

## Planned positive window

* Evidence start: 2025-06-23 22:05:11 UTC
* Cutoff: 2026-06-23 22:05:11 UTC
* Target: 2026-06-24 22:05:11 UTC
* Forecast lead time: 24 hours
* Expected label: positive

## Planned controls

* Venezuela 2022 annual control
* Venezuela 2023 annual control
* Venezuela 2024 annual control
* Venezuela 2025 annual control

Every window will be classified using the frozen warning threshold of
0.73.

## Interpretation

CAMEFF hazard evidence is an uncalibrated evidence score and not an
earthquake-occurrence probability.

This is an untouched retrospective generalization test.

## Run the positive hindcast

From the repository root:

```bash
./experiments/venezuela_2026/run_hindcast.sh
```

The positive experiment uses a cutoff exactly twenty-four hours before
the M7.5 target event.

The generated report is written to:

```text
experiments/venezuela_2026/results/venezuela_2026_hindcast.txt
```

## Run the validation suite

Run four annual controls followed by the positive target window:

```bash
./experiments/venezuela_2026/run_validation_suite.sh
```

Inspect the results:

```bash
column -s, -t \
  experiments/venezuela_2026/results/venezuela_validation_windows.csv
```

The `predicted_label` field uses the frozen warning threshold of 0.73:

* `1` means CAMEFF issued a warning.
* `0` means the hazard score remained below the threshold.

## Frozen-threshold metrics

Generate the Venezuela metrics:

```bash
./experiments/venezuela_2026/run_threshold_report.sh
```

Inspect the metrics:

```bash
column -s, -t \
  experiments/venezuela_2026/results/venezuela_frozen_threshold_metrics.csv
```

The threshold remains fixed at 0.73. A positive score below this value
must be recorded as a miss.
