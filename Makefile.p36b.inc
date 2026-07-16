.PHONY: real-catalog-mc-parity real-catalog-parity

real-catalog-mc-parity: all
	python3 tests/real_catalog_parity/run_mc_parity.py

real-catalog-parity: all
	python3 tests/real_catalog_parity/run_parity.py --repo-root .
