#ifndef MC_REG_WORLD_CONFIG_H
#define MC_REG_WORLD_CONFIG_H

#include "config/config.h"

EMP_BUILD_CONFIG(MCRegConfig,
  GROUP(DEFAULT_GROUP, "General settings"),
    VALUE(SEED, int, 0, "Random number seed (-1 for seed based on time)"),
    VALUE(GENERATIONS, size_t, 100, "How many generations do we evolve things?"),
    VALUE(POP_SIZE, size_t, 100, "How big is our population?"),
    VALUE(STOP_ON_SOLUTION, bool, true, "Should we stop run on solution?"),

)

#endif