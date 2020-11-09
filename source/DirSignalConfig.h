#ifndef DIR_SIG_WORLD_CONFIG_H
#define DIR_SIG_WORLD_CONFIG_H

#include "emp/config/config.hpp"

EMP_BUILD_CONFIG(DirSigConfig,
  GROUP(DEFAULT_GROUP, "General settings"),
    VALUE(SEED, int, 0, "Random number seed (-1 for seed based on time)"),
    VALUE(GENERATIONS, size_t, 100, "How many generations do we evolve things?"),
    VALUE(POP_SIZE, size_t, 100, "How big is our population?"),
    VALUE(STOP_ON_SOLUTION, bool, false, "Should we stop run on solution?"),

  GROUP(EVALUATION_GROUP, "Organism evaluation settings"),
    VALUE(EVAL_TRIAL_CNT, size_t, 1, "How many times should we evaluate individuals (where fitness = min trial performance)?"),

  GROUP(ENVIRONMENT_GROUP, "Environment settings"),
    VALUE(NUM_ENV_STATES, size_t, 8, "How many responses are there to the environment signal?"),
    VALUE(NUM_ENV_UPDATES, size_t, 8, "How many responses are there to the environment signal?"),
    VALUE(CPU_CYCLES_PER_ENV_UPDATE, size_t, 8, "How many CPU cycles do organisms get per environment time step?"),

  GROUP(PROGRAM_GROUP, "Program settings"),
    VALUE(USE_FUNC_REGULATION, bool, true, "Do programs have access to function regulation instructions?"),
    VALUE(USE_GLOBAL_MEMORY, bool, false, "Do programs have access to global memory?"),
    VALUE(MIN_FUNC_CNT, size_t, 0, "Minimum number of functions per program."),
    VALUE(MAX_FUNC_CNT, size_t, 32, "Maximum number of functions per program."),
    VALUE(MIN_FUNC_INST_CNT, size_t, 0, "Minimum number of instructions per function."),
    VALUE(MAX_FUNC_INST_CNT, size_t, 128, "Maximum number of instructions per function."),
    VALUE(INST_MIN_ARG_VAL, int, -8, "Minimum instruction-argment value"),
    VALUE(INST_MAX_ARG_VAL, int, 8, "Maximum instruction-argument value"),

  GROUP(HARDWARE_GROUP, "Virtual hardware settings"),
    VALUE(MAX_ACTIVE_THREAD_CNT, size_t, 16, "How many threads can be simultaneously running (active)?"),
    VALUE(MAX_THREAD_CAPACITY, size_t, 32, "Maximum capacity for thread memory (pending + active)."),

  GROUP(SELECTION_GROUP, "Selection settings"),
    VALUE(SELECTION_MODE, std::string, "lexicase", "Selection scheme? [lexicase, tournament]"),
    VALUE(TOURNAMENT_SIZE, size_t, 8, "How big are tournaments when doing tournament selection?"),
    VALUE(TEST_SAMPLE_SIZE, size_t, 1, "How many directional sequences should we sample to evaluate organisms?"),

  GROUP(MUTATION_GROUP, "Mutation settings"),
    VALUE(MUT_RATE__INST_ARG_SUB, double, 0.005, "InstArgSub rate"),
    VALUE(MUT_RATE__INST_SUB, double, 0.005, "InstSub rate"),
    VALUE(MUT_RATE__INST_INS, double, 0.005, "InstIns rate"),
    VALUE(MUT_RATE__INST_DEL, double, 0.005, "InstDel rate"),
    VALUE(MUT_RATE__SEQ_SLIP, double, 0.05, "SeqSlip rate"),
    VALUE(MUT_RATE__FUNC_DUP, double, 0.05, "FuncDup rate"),
    VALUE(MUT_RATE__FUNC_DEL, double, 0.05, "FuncDel rate"),
    VALUE(MUT_RATE__INST_TAG_BF, double, 0.001, "InstArgTagBF rate"),
    VALUE(MUT_RATE__FUNC_TAG_BF, double, 0.001, "FuncTagBF rate"),

  GROUP(DATA_COLLECTION_GROUP, "Data collection settings"),
    VALUE(OUTPUT_DIR, std::string, "output", "where should we dump output?"),
    VALUE(MINIMAL_TRACES, bool, true, "how extensively should we collect program traces on input sequences?"),
    VALUE(SUMMARY_RESOLUTION, size_t, 10, "How often should we output summary statistics?"),
    VALUE(SCREEN_RESOLUTION, size_t, 10, "How often should we attempt to screen for a solution?"),
    VALUE(SNAPSHOT_RESOLUTION, size_t, 100, "How often should we snapshot the population?"),
)

#endif