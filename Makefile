# Target project
# PROJECT options: alt-signal-exp, chg-env-exp, dir-signal-exp
PROJECT := bool-calc-exp
# - Repeated signal task -
# PROJECT := alt-signal-exp
# - Changing signal task -
# PROJECT := chg-env-exp
# - Directional signal task -
# PROJECT := dir-signal-exp

# Dependency directories
EMP_DIR := ../Empirical/include		# Path to Empirical include directory.
SGP_DIR := ../SignalGP/source			# Path to SignalGP source directory.

# Compile-time parameter configuration (tag metric, matching threshold, matching regulator, tag size)
# MATCH_METRIC options: hamming, hash, integer, integer-symmetric, streak, streak-exact
MATCH_METRIC := streak
# MATCH_THRESH options: 0, 25, 50, 75
MATCH_THRESH := 0
# MATCH_REG options: add, mult, exp
MATCH_REG := exp
# TAG_NUM_BITS
TAG_NUM_BITS := 256

# Executable name
# combine it all into the executable name
EXEC_NAME := $(PROJECT)_tag-len-$(TAG_NUM_BITS)_match-metric-$(MATCH_METRIC)_thresh-$(MATCH_THRESH)_reg-$(MATCH_REG)

# Flags to use regardless of compiler
# CFLAGS_openssl := -I$(OPEN_SSL_DIR)/include -L$(OPEN_SSL_DIR)/lib
CFLAGS_includes := -I./source/ -I$(EMP_DIR)/ -I$(SGP_DIR)/
CFLAGS_links := -lssl -lcrypto
CFLAGS_all := -Wall -Wno-unused-function -pedantic -std=c++17 -DEMP_HAS_CRYPTO=1 -DMATCH_METRIC=$(MATCH_METRIC) -DMATCH_THRESH=$(MATCH_THRESH) -DMATCH_REG=$(MATCH_REG) -DTAG_NUM_BITS=$(TAG_NUM_BITS) $(CFLAGS_openssl) $(CFLAGS_includes) $(CFLAGS_links)

# Native compiler information
CXX_nat := g++
CFLAGS_nat := -O3 -DNDEBUG $(CFLAGS_all)
CFLAGS_nat_debug := -g $(CFLAGS_all)

default: $(PROJECT)
native: $(PROJECT)
all: $(PROJECT)

debug:	CFLAGS_nat := $(CFLAGS_nat_debug)
debug:	$(PROJECT)

$(PROJECT):	source/native/$(PROJECT).cc
	$(CXX_nat) $(CFLAGS_nat) source/native/$(PROJECT).cc -o $(EXEC_NAME)

clean:
	rm -rf $(PROJECT)_*.dSYM
	rm -f $(PROJECT) $(PROJECT)_tag-len-*_match-metric-* *~ source/*.o test_debug.out test_optimized.out unit_tests.gcda unit_tests.gcno
	rm -rf test_debug.out.dSYM

test: clean
test: tests/unit_tests.cc
	$(CXX_nat) $(CFLAGS_nat_debug) -I./source/ --coverage tests/unit_tests.cc -o test_debug.out
	./test_debug.out

# Debugging information
print-%: ; @echo '$(subst ','\'',$*=$($*))'
