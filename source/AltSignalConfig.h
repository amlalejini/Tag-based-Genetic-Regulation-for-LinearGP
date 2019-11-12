#ifndef ALT_SIGNAL_WORLD_CONFIG_H
#define ALT_SIGNAL_WORLD_CONFIG_H

#include "config/config.h"

EMP_BUILD_CONFIG(AltSignalConfig,
  GROUP(DEFAULT_GROUP, "General settings"),
    VALUE(SEED, int, 0, "Random number seed (-1 for seed based on time)"),
    VALUE(GENERATIONS, size_t, 100, "How many generations do we evolve things?"),
    VALUE(POP_SIZE, size_t, 100, "How big is our population?"),

  GROUP(PROGRAM_GROUP, "Program settings"),

  GROUP(HARDWARE_GROUP, "Virtual hardware settings"),

  GROUP(SELECTION_GROUP, "Selection settings"),

  GROUP(DATA_COLLECTION_GROUP, "Data collection settings"),
)

#endif