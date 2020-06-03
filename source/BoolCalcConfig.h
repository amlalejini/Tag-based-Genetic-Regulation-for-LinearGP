#ifndef BOOL_CALC_WORLD_CONFIG_H
#define BOOL_CALC_WORLD_CONFIG_H

#include "config/config.h"

EMP_BUILD_CONFIG(BoolCalcConfig,
  GROUP(DEFAULT_GROUP, "General Settings"),
    VALUE(SEED, int, 0, "Random number seed (-1 for seed based on time)"),
    VALUE(GENERATIONS, size_t, 100, "How many generations should we evolve programs?"),
    VALUE(POP_SIZE, size_t, 100, "How many individuals are in our population?"),
    VALUE(STOP_ON_SOLUTION, bool, true, "Should we stop run on solution?"),

  GROUP(EVALUATION_GROUP, "Evaluation settings"),
    VALUE(TESTING_SET_FILE, std::string, "./test_cases.csv", "Path to the csv containing test cases to use to evaluate programs."),
    VALUE(TRAINING_SET_FILE, std::string, "./training_cases.csv", "Path to the csv containing training test cases to use to determine if a program is a solution."),

  GROUP(SELECTION_GROUP, "Selection settings"),
    VALUE(DOWN_SAMPLE, bool, 0, "Should we down-sample the testing set for evaluation?"),
    VALUE(DOWN_SAMPLE_RATE, double, 0.25, "What proportion of the test cases should we use each generation?"),

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
    VALUE(SUMMARY_RESOLUTION, size_t, 10, "How often should we output summary statistics?"),
    VALUE(SNAPSHOT_RESOLUTION, size_t, 100, "How often should we snapshot the population?"),

)

#endif