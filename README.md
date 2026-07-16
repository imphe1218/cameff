# CAMEFF P36A Real-Catalog Python–C17 Parity

This patch adds real-catalog parity testing to the released
`fm-v1.4-c17-reference-1.0.0` repository.

## Executed result

- Ready cases: 4
- Passed: 1
- Failed: 3
- Blocked: 1
- Overall: `FAIL`

Japan, Cebu, Davao, and Venezuela were executed using the supplied frozen USGS
catalog package. Russia is registered but blocked because its frozen catalog was
not included in that package.

No CAMEFF mathematics or C17 implementation source was changed.

## Apply to the repository

Copy `tests/real_catalog_parity/` into the repository's `tests/` directory and
append `Makefile.p36a.inc` to the root `Makefile`.

Then run:

```sh
make clean all
make real-catalog-parity
```

The included `acquire_russia_catalog.sh` downloads the missing Russia catalog
from the official USGS FDSN service when run in an internet-enabled environment.
After acquisition, the catalog must be frozen, hashed, preprocessed, and added
to the case manifest before the five-case gate can be declared complete.
