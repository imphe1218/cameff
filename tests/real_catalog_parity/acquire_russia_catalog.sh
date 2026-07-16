#!/usr/bin/env bash
set -euo pipefail

OUT="${1:-tests/real_catalog_parity/catalogs/russia_2025_usgs.csv}"

curl --fail --location --silent --show-error \
  'https://earthquake.usgs.gov/fdsnws/event/1/query?format=csv&orderby=time-asc&eventtype=earthquake&starttime=2000-01-01T00%3A00%3A00Z&endtime=2025-07-29T23%3A24%3A52Z&minmagnitude=3.0&latitude=52.495&longitude=160.240&maxradiuskm=600&limit=20000' \
  -o "$OUT"

sha256sum "$OUT"
