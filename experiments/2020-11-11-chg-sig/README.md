# 2020-11-11 - Changing-signal problem

## Directory guide:

- analysis/
  - Contains analysis scripts, data, and graphs/visualizations for this experiment.
- hpcc/
  - Contains configuration files and SLURM job submission scripts for this experiment.

## Overview

In this experiment, we compare the problem-solving success of regulation-enabled SignalGP with that of regulation-disabled SignalGP.

- Environment settings
  - `-NUM_ENV_STATES 16 -NUM_ENV_UPDATES 16`
- Experimental conditions:
  - Regulation-disabled: `-USE_FUNC_REGULATION 0 -USE_GLOBAL_MEMORY 1`
  - Regulation-enabled: `-USE_FUNC_REGULATION 1 -USE_GLOBAL_MEMORY 1`
