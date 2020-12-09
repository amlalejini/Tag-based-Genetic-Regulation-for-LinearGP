# 2020-11-25 - Repeated-signal problem

## Directory guide:

- analysis/
  - Contains analysis scripts, data, and graphs/visualizations for this experiment.
- hpcc/
  - Contains configuration files and SLURM job submission scripts for this experiment.

## Overview

In this experiment, we compare the problem-solving success of regulation-enabled SignalGP with that of regulation-disabled SignalGP on the repeated-signal problem (at four difficulty levels).

- Environment settings
  - two-signal problem: `-NUM_SIGNAL_RESPONSES 2 -NUM_ENV_CYCLES 2`
  - four-signal problem: `-NUM_SIGNAL_RESPONSES 4 -NUM_ENV_CYCLES 4`
  - eight-signal problem: `-NUM_SIGNAL_RESPONSES 8 -NUM_ENV_CYCLES 8`
  - sixteen-signal problem: `-NUM_SIGNAL_RESPONSES 16 -NUM_ENV_CYCLES 16`
- Experimental conditions:
  - Regulation-disabled: `-USE_FUNC_REGULATION 0 -USE_GLOBAL_MEMORY 1`
  - Regulation-enabled: `-USE_FUNC_REGULATION 1 -USE_GLOBAL_MEMORY 1`