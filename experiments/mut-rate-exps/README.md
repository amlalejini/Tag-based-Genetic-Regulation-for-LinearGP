# Mutation Rate Experiments

DISCLAIMER: the underlying source code has changed since running these exploratory experiments, as such
the data aggregation script and configuration files may not work with the current C++ implementations.

Goal: small sweep of mutation rates for tags for several different tag matching
metrics.

- Mutation rates (per-bit): 0.0005, 0.001, 0.002, 0.005
- Match metrics: streak, hamming, integer
- Function regulation: all runs
- Environment
  - NUM_SIGNAL_RESPONSES: 2, 4, 8, 16
  - NUM_ENV_CYCLES: 2*NUM_SIGNAL_RESPONSES
- Replicates? 30

## Conclusion

- Streak is generally best,
  - For 64-bit tags, 0.001 seems best
  - Minimum match threshold? 25%

See data analyses here: [./analysis/mut-rate-exps.html](./analysis/mut-rate-exps.html)