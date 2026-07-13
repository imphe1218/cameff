# Russia 2025 Kamchatka Hindcast Experiment

This experiment evaluates the frozen CAMEFF b1.0.0 hindcast pipeline
against the 2025 Kamchatka Peninsula earthquake.

## Target event

* Time: 2025-07-29 23:24:52 UTC
* Latitude: 52.495 degrees north
* Longitude: 160.240 degrees east
* Depth: 35.0 km
* Magnitude: M8.8
* Tectonic setting: subduction megathrust
* USGS event identifier: us6000qw60

## Frozen validation rules

The rules inherited from the Japan validation are frozen before this
experiment is evaluated:

* Warning threshold: 0.73
* Forecast lead time: 24 hours
* Evidence window: 365 days
* Analysis radius: 500 km
* Minimum catalog magnitude: M2.5
* Minimum evidence events: 20

The warning threshold must not be adjusted to fit the Russia result.

## Validation catalog

The extended catalog covers approximately five years and supports four
annual control windows followed by the positive 2025 window.

Acquisition parameters:

* Start: 2020-07-28 23:24:52 UTC
* End: 2025-07-29 23:24:51 UTC
* Centre: 52.495, 160.240
* Radius: 500 km
* Minimum magnitude: M2.5
* Event type: earthquake
* Ordering: ascending origin time

The raw catalog ends one second before the target earthquake, preventing
the target event from entering the evidence data.

## Download

From the repository root:

```bash
./experiments/russia_2025/fetch_usgs_raw.sh
```

The raw catalog is written to:

```text
experiments/russia_2025/data/raw/usgs_kamchatka_2020_2025_raw.csv
```

## Normalize

Build the utilities:

```bash
cmake --build build
```

Normalize the catalog:

```bash
./experiments/russia_2025/normalize_catalog.sh
```

The normalized catalog is written to:

```text
experiments/russia_2025/data/normalized/catalog.csv
```

## Planned positive window

* Evidence start: 2024-07-28 23:24:52 UTC
* Cutoff: 2025-07-28 23:24:52 UTC
* Target: 2025-07-29 23:24:52 UTC
* Lead time: 24 hours
* Expected label: positive

## Planned controls

* 2021 annual control
* 2022 annual control
* 2023 annual control
* 2024 annual control

The control labels must be verified against the catalog before computing
final metrics.

## Interpretation

CAMEFF hazard evidence remains an uncalibrated evidence score. The Russia
case is an untouched generalization test of the threshold derived from
the Japan experiment.
