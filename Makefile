CC ?= cc
AR ?= ar

CFLAGS ?= -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror
CPPFLAGS ?= -Iinclude -Ithird_party/scipy_lbfgsb
LDLIBS ?= -llapack -lblas -lm

BUILD := build
LIBOBJ := $(BUILD)/cameff.o $(BUILD)/lbfgsb_standalone.o

.PHONY: all test equivalence-test sanitize clean

all: $(BUILD)/libcameff.a $(BUILD)/cameff_cli $(BUILD)/test_cameff

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/cameff.o: src/cameff.c include/cameff/cameff.h third_party/scipy_lbfgsb/lbfgsb_standalone.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/cameff.c -o $@

$(BUILD)/lbfgsb_standalone.o: third_party/scipy_lbfgsb/lbfgsb_standalone.c third_party/scipy_lbfgsb/lbfgsb_standalone.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c third_party/scipy_lbfgsb/lbfgsb_standalone.c -o $@

$(BUILD)/libcameff.a: $(LIBOBJ)
	$(AR) rcs $@ $^

$(BUILD)/cameff_cli: tools/cameff_cli.c $(BUILD)/libcameff.a
	$(CC) $(CPPFLAGS) $(CFLAGS) tools/cameff_cli.c $(BUILD)/libcameff.a $(LDLIBS) -o $@

$(BUILD)/test_cameff: tests/unit/test_cameff.c $(BUILD)/libcameff.a
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/unit/test_cameff.c $(BUILD)/libcameff.a $(LDLIBS) -o $@

test: all
	./$(BUILD)/test_cameff

equivalence-test: all
	python3 tests/equivalence/run_equivalence.py

sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS="-std=c17 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined" all
	./$(BUILD)/test_cameff

clean:
	rm -rf $(BUILD)
.PHONY: real-catalog-mc-parity real-catalog-parity

real-catalog-mc-parity: all
	python3 tests/real_catalog_parity/run_mc_parity.py

real-catalog-parity: all
	python3 tests/real_catalog_parity/run_parity.py --repo-root .
