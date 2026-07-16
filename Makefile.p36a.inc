.PHONY: real-catalog-parity

real-catalog-parity: all
	python3 tests/real_catalog_parity/run_parity.py --repo-root .
