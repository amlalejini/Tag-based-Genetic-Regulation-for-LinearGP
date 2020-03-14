# Experiment directory

This directory contains files related to exploratory/preliminary experiments and to the final set of
experiments for our ALife 2020 contribution.

## Exploratory experiments

### Mutation rate experiments (11/26ish)

Small sweep of mutation rates for tags for several different tag matching
metrics.

Goal: settle on a tag mutation rate and matching metric to use going forward.

[See experiment readme for details.](./mut-rate-exps/)

Post-experiment, major changes to matchbin regulator. Need to re-write regulation instructions & run
add vs multiplicative regulator experiment (to pick one to use going forward).

Results: using the streak matching metric going forward.

### Regulation type experiments (12/17)

Goal: look at multiplicative vs additive module regulation to decide which to use going forward.

[See experiment readme for more details.](./reg-type-exps/)

Result: using the multiplicative regulator going forward.

### Demonstrating regulation experiment

Goal: demonstrate function regulation in SignalGP, figure out what experiments to run for the paper.

[See experiment readme for more details.](./dem-reg-exps/)

## ALife 2020 experiments

[See experiment readme for more details.](./alife-2020/)
