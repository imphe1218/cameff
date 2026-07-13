# Japan 2011 Hindcast Experiment

This experiment evaluates the CAMEFF b1.0.0 hindcast pipeline for the
2011 Great Tohoku earthquake.

## Target event

* Time: 2011-03-11 05:46:24 UTC
* Latitude: 38.297 degrees north
* Longitude: 142.373 degrees east
* Depth: 29 km
* Magnitude: M9.1
* Tectonic setting: subduction

## Precursor catalog

The raw catalog is downloaded from the USGS FDSN Event Web Service.

Acquisition parameters:

* Start: 2006-03-10 05:46:24 UTC
* End: 2011-03-11 05:46:23 UTC
* Centre: 38.297, 142.373
* Radius: 500 km
* Minimum magnitude: 2.5
* Event type: earthquake
* Ordering: ascending origin time

The end time is one second before the target earthquake. This prevents
the target event from being included in the precursor catalog.

## Download

From the repository root:

```bash
./experiments/japan_2011/fetch_usgs_raw.sh
```

The downloaded raw catalog is written to:

```text
experiments/japan_2011/data/raw/usgs_japan_2006_2011_raw.csv
```

## Normalize the USGS catalog

Build the CAMEFF utilities:

```bash
cmake --build build
```

Normalize the downloaded USGS catalog:

```bash
./experiments/japan_2011/normalize_catalog.sh
```

The normalized catalog is written to:

```text
experiments/japan_2011/data/normalized/catalog.csv
```

The normalized schema is:

```csv
time,latitude,longitude,depth,mag,region
```

The normalizer resolves columns by their USGS header names instead of
assuming fixed column positions. Quoted place names containing commas are
preserved correctly.

## Validation windows

The extended catalog supports repeated one-year evidence windows under
the same regional and catalog settings.

Planned windows:

| Window | Evidence start | Cutoff | Outcome |
|---|---|---|---|
| Japan 2007 control | 2006-03-10 | 2007-03-10 | Negative control |
| Japan 2008 control | 2007-03-10 | 2008-03-10 | Negative control |
| Japan 2009 control | 2008-03-10 | 2009-03-10 | Negative control |
| Japan 2010 control | 2009-03-10 | 2010-03-10 | Negative control |
| Japan 2011 positive | 2010-03-10 | 2011-03-10 | Positive |

The positive cutoff is twenty-four hours before the 2011 target
earthquake.

All windows use:

- Centre: 38.297 degrees north, 142.373 degrees east
- Radius: 500 km
- Lookback: 365 days
- Minimum magnitude in the source catalog: M2.5
- Minimum evidence events: 20

The negative labels and warning threshold will be frozen before final
performance metrics are reported.

## Run the hindcast

Build the project:

```bash
cmake --build build
```

Run the reproducible Japan 2011 experiment:

```bash
./experiments/japan_2011/run_hindcast.sh
```

The generated report is written to:

```text
experiments/japan_2011/results/japan_2011_hindcast.txt
```

The target earthquake is supplied separately from the precursor catalog.

The precursor catalog ends before the target event. This prevents the
target event record, magnitude, and depth from entering the signal
extraction pipeline as precursor evidence.

## Data stages

```text
USGS raw CSV
    -> CAMEFF normalized CSV
    -> leakage-free hindcast window
    -> signal extraction
    -> pattern classification
    -> expert fusion
    -> experiment report
```

## Reproducible workflow

From the repository root:

```bash
./experiments/japan_2011/fetch_usgs_raw.sh
./experiments/japan_2011/normalize_catalog.sh
./experiments/japan_2011/run_hindcast.sh
```

## Interpretation warning

CAMEFF hazard evidence is an uncalibrated evidence score.

It must not be interpreted as the probability that an earthquake will
occur. The experiment is a historical hindcast used to evaluate signal
processing, pattern classification, expert routing, and fusion behavior.
