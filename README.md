# CAMEFF C17 Reference Implementation — FM-v1.4

This repository contains the simple standalone C17 operational numerical
reference implementation of the validated CAMEFF FM-v1.4 branch.

## Implemented branch

- magnitude-completeness estimation;
- Gutenberg–Richter b-value;
- temporal ETAS fitting;
- official SciPy L-BFGS-B C translation;
- stationarity rejection;
- seven-day M>=6 probability;
- P29K calibration support;
- architecture utilities for quality, unknown support, and fusion.

## Build

```sh
make clean all
```

## Test

```sh
make test
make equivalence-test
```

The equivalence suite contains 36 frozen catalog fixtures and uses the P34K
mixed absolute/relative near-zero comparison contract.

Expected result:

```text
36 cases
36 passed
0 failed
```

## Dependencies

- ISO C17 compiler
- BLAS
- LAPACK
- Python 3 only for the equivalence test harness

Ubuntu/Debian:

```sh
sudo apt-get install build-essential libblas-dev liblapack-dev python3
```

## Reference status

This is an operational numerical one-to-one implementation of the validated
FM-v1.4 ETAS branch under the frozen P34F/P34K numerical contracts.

Bit-for-bit equality between Python and C17 is not claimed.

Binary and public-warning outputs remain disabled.
