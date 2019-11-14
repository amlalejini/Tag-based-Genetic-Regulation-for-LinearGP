#ifndef ALT_SIGNAL_WORLD_CONFIG_H
#define ALT_SIGNAL_WORLD_CONFIG_H

#include "config/config.h"

EMP_BUILD_CONFIG(AltSignalConfig,
  GROUP(DEFAULT_GROUP, "General settings"),
    VALUE(SEED, int, 0, "Random number seed (-1 for seed based on time)"),
    VALUE(GENERATIONS, size_t, 100, "How many generations do we evolve things?"),
    VALUE(POP_SIZE, size_t, 100, "How big is our population?"),

  GROUP(PROGRAM_GROUP, "Program settings"),
    VALUE(MIN_FUNC_CNT, size_t, 0, "Minimum number of functions per program."),
    VALUE(MAX_FUNC_CNT, size_t, 32, "Maximum number of functions per program."),
    VALUE(MIN_FUNC_INST_CNT, size_t, 128, "Minimum number of instructions per function."),
    VALUE(MAX_FUNC_INST_CNT, size_t, 128, "Maximum number of instructions per function."),
  GROUP(HARDWARE_GROUP, "Virtual hardware settings"),

  GROUP(SELECTION_GROUP, "Selection settings"),

  GROUP(DATA_COLLECTION_GROUP, "Data collection settings"),
)

#endif