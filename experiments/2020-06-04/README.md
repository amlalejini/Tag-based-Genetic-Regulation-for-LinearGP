# 2020-06-04 - Exploratory Experiments

Goal: Exploratory experiments with the freshly implemented Boolean Calculator World

## Ideas

- Add double echo? Add some sort of each building block task that requires echoing back first number (& second?)
- Add possibility for large-impact tag mutation (randomization, slip)
- Increase tag size
- Decrease tag mutation rate?
- Decrease instruction-level mutation rate
- Implement/add more support for hierarchical/batch regulation
  - ability to regulate chunks all at once/together

## 2020-06-04 - Explorations

- Configuration
  - seed offset: 1000
  - See [./config-2020-06-04.cfg](./config-2020-06-04.cfg) for configuration
  - Ran with the full set of logic tasks.
- Results
  - Over representation of `ERROR_OP_OP` test cases
  - That's really the only thing programs are solving.
  - ~2 days to hit 5k generations
  - No solutions.

## 2020-06-06 - Explorations

- Changes
  - seed offset: 2000
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
  - Seed offset: 3000
  - Infix to postfix notation for test cases
  - Reduce tag mutation rate (0.002 to 0.0001)
  - Increase tag size (64 to 128)
  - Increase max number of functions (64 to 128)
  - Increase number of training cases, reduce down sample rate (0.1 to 0.05)

## 2020-06-11 - Explorations

- Changes
  - Seed offset: 4000
  - Add all logic tasks
  - Fix bug where global memory was getting reset inappropriately
