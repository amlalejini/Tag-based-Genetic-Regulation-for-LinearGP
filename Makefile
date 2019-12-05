# Project-specific settings
PROJECT := signalgp-genetic-regulation
EMP_DIR := ../Empirical/source
SGP_DIR := ../SignalGP/source
# CEREAL_DIR := ../Empirical/third-party/cereal/include

MATCH_METRIC := hamming

# Flags to use regardless of compiler
CFLAGS_includes := -I./source/ -I$(EMP_DIR)/ -I$(SGP_DIR)/ -I$(CEREAL_DIR)/
CFLAGS_all := -Wall -Wno-unused-function -pedantic -std=c++17 -DMATCH_METRIC=$(MATCH_METRIC) $(CFLAGS_includes)

# Native compiler information
CXX_nat := g++-9
CFLAGS_nat := -O3 -DNDEBUG $(CFLAGS_all)
CFLAGS_nat_debug := -g $(CFLAGS_all)

# Emscripten compiler information
CXX_web := emcc
OFLAGS_web_all := -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap']" -s TOTAL_MEMORY=67108864 --js-library $(EMP_DIR)/web/library_emp.js -s EXPORTED_FUNCTIONS="['_main', '_empCppCallback']" -s DISABLE_EXCEPTION_CATCHING=1 -s NO_EXIT_RUNTIME=1 #--embed-file configs
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
	$(CXX_nat) $(CFLAGS_nat) source/native/$(PROJECT).cc -o $(PROJECT)_match-metric-$(MATCH_METRIC)
	@echo To build the web version use: make web

$(PROJECT).js: source/web/$(PROJECT)-web.cc
	$(CXX_web) $(CFLAGS_web) source/web/$(PROJECT)-web.cc -o web/$(PROJECT).js

.PHONY: clean test serve

serve:
	python3 -m http.server

clean:
	rm -rf signalgp-genetic-regulation_*.dSYM
	rm -f $(PROJECT) $(PROJECT)_match-metric-* web/$(PROJECT).js web/*.js.map web/*.js.map *~ source/*.o web/*.wasm web/*.wast test_debug.out test_optimized.out unit_tests.gcda unit_tests.gcno
	rm -rf test_debug.out.dSYM

test: clean
test: tests/unit_tests.cc
	$(CXX_nat) $(CFLAGS_nat_debug) -I./source/ --coverage tests/unit_tests.cc -o test_debug.out
	./test_debug.out

# Debugging information
print-%: ; @echo '$(subst ','\'',$*=$($*))'
