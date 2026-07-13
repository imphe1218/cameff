# Japan 2011 Hindcast Experiment

This experiment evaluates the CAMEFF b1.0.0 hindcast pipeline for the
2011 Great Tohoku earthquake.

## Target event

- Time: 2011-03-11 05:46:24 UTC
- Latitude: 38.297 degrees north
- Longitude: 142.373 degrees east
- Depth: 29 km
- Magnitude: M9.1
- Tectonic setting: subduction

## Precursor catalog

The raw catalog is downloaded from the USGS FDSN Event Web Service.

Acquisition parameters:

- Start: 2010-03-11 05:46:24 UTC
- End: 2011-03-11 05:46:23 UTC
- Centre: 38.297, 142.373
- Radius: 500 km
- Minimum magnitude: 2.5
- Event type: earthquake
- Ordering: ascending origin time

The end time is one second before the target earthquake. This prevents
the target event from being included in the precursor catalog.

## Download

From the repository root:

```bash
./experiments/japan_2011/fetch_usgs_raw.sh

## Normalize the USGS catalog

Build the CAMEFF utilities:

```bash
cmake --build build