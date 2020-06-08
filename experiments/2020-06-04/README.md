# 2020-06-04 - Exploratory Experiments

Goal: Exploratory experiments with the freshly implemented Boolean Calculator World

## 2020-06-04 - Explorations

- Configuration
  - See [./config-2020-06-04.cfg](./config-2020-06-04.cfg) for configuration
  - Ran with the full set of logic tasks.
- Results
  - Over representation of `ERROR_OP_OP` test cases
  - That's really the only thing programs are solving.
  - ~2 days to hit 5k generations
  - No solutions.

## 2020-06-06 - Explorations

- Changes
  - Add test sampling by example type
  - Reduce number of tasks.
  - Reduce tag size
- Results
  - Maybe I need to lower the tag mutation rate?
  - Down-sampling by test type is good
  - Having trouble getting past ECHO
  - Runs not able to maintain tags (mutational meltdown on tags?)

## 2020-06-08 - Explorations

- Changes
  - Infix to postfix notation for test cases
  - Reduce tag mutation rate (to 0.0001)
  - Increase tag size (to 128)
