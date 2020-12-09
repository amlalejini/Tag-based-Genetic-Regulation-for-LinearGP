# 2020-11-28 - Boolean logic calculator problem (postfix notation)

## Directory guide:

- analysis/
  - Contains analysis scripts, data, and graphs/visualizations for this experiment.
- hpcc/
  - Contains configuration files and SLURM job submission scripts for this experiment.

## Overview

In this experiment, we compare the problem-solving success of regulation-enabled SignalGP with that of regulation-disabled SignalGP on the boolean logic calculator (postfix notation) problem.

- IO examples (for the problem used in the paper)
  - training set ./hpcc/training_set_postfix.csv
  - testing set: ./hpcc/testing_set_postfix.csv
- Experimental conditions:
  - Regulation-disabled: `-USE_FUNC_REGULATION 0 -USE_GLOBAL_MEMORY 1`
  - Regulation-enabled: `-USE_FUNC_REGULATION 1 -USE_GLOBAL_MEMORY 1`