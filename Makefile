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
EMP_DIR := ../Empirical/include
SGP_DIR := ../SignalGP/source

# Compile-time parameter configuration (tag metric, matching threshold, matching regulator, tag size)
# MATCH_METRIC options: hamming, hash, integer, integer-symmetric, streak, streak-exact
MATCH_METRIC := streak
# MATCH_THRESH options: 0, 25, 50, 75
MATCH_THRESH := 0
# MATCH_REG options: add, mult
MATCH_REG := mult
# TAG_NUM_BITS
TAG_NUM_BITS := 128

# Executable name
# combine it all into the executable name
EXEC_NAME := $(PROJECT)_tag-len-$(TAG_NUM_BITS)_match-metric-$(MATCH_METRIC)_thresh-$(MATCH_THRESH)_reg-$(MATCH_REG)

# Flags to use regardless of compiler
# CFLAGS_openssl := -I$(OPEN_SSL_DIR)/include -L$(OPEN_SSL_DIR)/lib
CFLAGS_includes := -I./source/ -I$(EMP_DIR)/ -I$(SGP_DIR)/
CFLAGS_links := -lssl -lcrypto
CFLAGS_all := -Wall -Wno-unused-function -pedantic -std=c++17 -DMATCH_METRIC=$(MATCH_METRIC) -DMATCH_THRESH=$(MATCH_THRESH) -DMATCH_REG=$(MATCH_REG) -DTAG_NUM_BITS=$(TAG_NUM_BITS) $(CFLAGS_openssl) $(CFLAGS_includes) $(CFLAGS_links)

# Native compiler information
CXX_nat := g++
CFLAGS_nat := -O3 -DNDEBUG $(CFLAGS_all)
CFLAGS_nat_debug := -g $(CFLAGS_all)

# Emscripten compiler information
CXX_web := emcc
OFLAGS_web_all := -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap']" -s TOTAL_MEMORY=67108864 --js-library $(EMP_DIR)/emp/web/library_emp.js -s EXPORTED_FUNCTIONS="['_main', '_empCppCallback']" -s DISABLE_EXCEPTION_CATCHING=1 -s NO_EXIT_RUNTIME=1 #--embed-file configs
OFLAGS_web := -Oz -DNDEBUG
OFLAGS_web_debug := -g4 -Oz -Wno-dollar-in-identifier-extension

CFLAGS_web := $(CFLAGS_all) $(OFLAGS_web) $(OFLAGS_web_all)
CFLAGS_web_debug := $(CFLAGS_all) $(OFLAGS_web_debug) $(OFLAGS_web_all)

default: $(PROJECT)
native: $(PROJECT)
web: $(PROJECT).js
all: $(PROJECT) $(PROJECT).js

debug:	CFLAGS_nat := $(CFLAGS_nat_debug)
debug:	$(PROJECT)

debug-web:	CFLAGS_web := $(CFLAGS_web_debug)
debug-web:	$(PROJECT).js

web-debug:	debug-web

$(PROJECT):	source/native/$(PROJECT).cc
	$(CXX_nat) $(CFLAGS_nat) source/native/$(PROJECT).cc -o $(EXEC_NAME)
	@echo To build the web version use: make web

$(PROJECT).js: source/web/$(PROJECT)-web.cc
	$(CXX_web) $(CFLAGS_web) source/web/$(PROJECT)-web.cc -o web/$(PROJECT).js

.PHONY: clean test serve

serve:
	python3 -m http.server

clean:
	rm -rf $(PROJECT)_*.dSYM
	rm -f $(PROJECT) $(PROJECT)_match-metric-* web/$(PROJECT).js web/*.js.map web/*.js.map *~ source/*.o web/*.wasm web/*.wast test_debug.out test_optimized.out unit_tests.gcda unit_tests.gcno
	rm -rf test_debug.out.dSYM

test: clean
test: tests/unit_tests.cc
	$(CXX_nat) $(CFLAGS_nat_debug) -I./source/ --coverage tests/unit_tests.cc -o test_debug.out
	./test_debug.out

# Debugging information
print-%: ; @echo '$(subst ','\'',$*=$($*))'
