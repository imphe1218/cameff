# Apply P36B

**Source file:** `src/cameff.c`  
**Location:** replace the existing file with `src/cameff.c` from this package.

**Source file:** `tools/cameff_cli.c`  
**Location:** replace the existing file with `tools/cameff_cli.c` from this package.

Copy `tests/real_catalog_parity/` into the repository `tests/` directory.

Append `Makefile.p36b.inc` to the root `Makefile`. Remove the earlier P36A
`real-catalog-parity` target first if it is already present, to avoid duplicate
Make targets.

Run:

```sh
make clean all
make test
make equivalence-test
make real-catalog-mc-parity
make real-catalog-parity
```
