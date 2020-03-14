# Experiments on Regulation Type

DISCLAIMER: the underlying source code has changed since running these exploratory experiments, as such
the data aggregation script and configuration files may not work with the current C++ implementations.

Goal: decide which regulation type to use going forward: additive vs. multiplicative

- Mutation rates (per-bit): 0.001
- Match metrics: streak
- Minimum match threshold: 25%
- Function regulation: all runs
- Environment
  - NUM_SIGNAL_RESPONSES: 4, 8, 16, 32
  - NUM_ENV_CYCLES: 2*NUM_SIGNAL_RESPONSES
- Replicates? 30
- Regulation types: multiplicative, additive

Results: negligible difference between multiplicative and additive regulation. Going forward, we'll
use multiplicative.

See data analyses here: [./analysis/reg-type-exps.html]