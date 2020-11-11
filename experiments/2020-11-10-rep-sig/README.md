# 2020-11-10 Repeated Signal Task

The repeated signal task provides a simple evaluation of a representation's capacity to evolve programs capable of adjusting their response to an input signal during execution.
In this experiment, we compare the capacity for regulation-enabled and regulation-disabled SignalGP to yield programs capable of solving the repeated signal task at a range of difficulty levels.

Configuration details:

- General
  - GENERATIONS 10000     # How many generations do we evolve things?
  - POP_SIZE 1000         # How big is our population?
- Environment
  - NUM_SIGNAL_RESPONSES 16      # How many responses are there to the environment signal?
  - NUM_ENV_CYCLES 16            # How many times does the environment cycle?
  - CPU_TIME_PER_ENV_CYCLE 128  # How many CPU steps does a SignalGP organism get during an environment cycle?
- Program
  - MIN_FUNC_CNT 0         # Minimum number of functions per program.
  - MAX_FUNC_CNT 256        # Maximum number of functions per program.
  - MIN_FUNC_INST_CNT 0    # Minimum number of instructions per function.
  - MAX_FUNC_INST_CNT 128  # Maximum number of instructions per function.
  - INST_MIN_ARG_VAL -4    # Minimum instruction-argment value
  - INST_MAX_ARG_VAL 4     # Maximum instruction-argument value
- Hardware
  - MAX_ACTIVE_THREAD_CNT 4  # How many threads can be simultaneously running (active)?
  - MAX_THREAD_CAPACITY 8    # Maximum capacity for thread memory (pending + active).
- Selection group
  - TOURNAMENT_SIZE 8        # How big are tournaments when doing tournament selection?
- Mutation group
  - MUT_RATE__INST_ARG_SUB 0.001
  - MUT_RATE__INST_SUB 0.001
  - MUT_RATE__INST_INS 0.001
  - MUT_RATE__INST_DEL 0.001
  - MUT_RATE__SEQ_SLIP 0.05
  - MUT_RATE__FUNC_DUP 0.05
  - MUT_RATE__FUNC_DEL 0.05
  - MUT_RATE__INST_TAG_BF 0.0001
  - MUT_RATE__FUNC_TAG_BF 0.0001

Variables:

- Regulation/memory
  - -USE_FUNC_REGULATION 1 -USE_GLOBAL_MEMORY 0
  - -USE_FUNC_REGULATION 0 -USE_GLOBAL_MEMORY 1
  - -USE_FUNC_REGULATION 1 -USE_GLOBAL_MEMORY 1
- Task difficulty
  - -NUM_SIGNAL_RESPONSES 2 -NUM_ENV_CYCLES 2
  - -NUM_SIGNAL_RESPONSES 4 -NUM_ENV_CYCLES 4
  - -NUM_SIGNAL_RESPONSES 8 -NUM_ENV_CYCLES 8
  - -NUM_SIGNAL_RESPONSES 16 -NUM_ENV_CYCLES 16
