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
  - Fix bug where global memory was getting reset inappropriately (making the task much harder?)
- Results
  - Still need to decrease tag mutation rate (or instruction mutation rate?)
  - Also maybe increase number of functions per organism?
  - 31/39 training passes => best seen

## 2020-06-15 - Explorations

- Changes
  - Seed offset: 5000
  - Increase max function count
  - MAX_FUNC_CNT: 128 to 256
  - tag size: 128 to 256
  - MUT_RATE__INST_ARG_SUB: 0.005 to 0.002
  - MUT_RATE__INST_SUB: 0.005 to 0.002
  - MUT_RATE__INST_INS: 0.005 to 0.002
  - MUT_RATE__INST_DEL: 0.005 to 0.002

## 2020-06-17 - Explorations

- Changes
  - Increase population size: 500=>1000
  - Seed offset: 6000
  - Lower mutation rate
  - MUT_RATE__INST_ARG_SUB 0.001
  - MUT_RATE__INST_SUB 0.001
  - MUT_RATE__INST_INS 0.001
  - MUT_RATE__INST_DEL 0.001
  - MUT_RATE__SEQ_SLIP 0.05
  - MUT_RATE__FUNC_DUP 0.05
  - MUT_RATE__FUNC_DEL 0.05
  - MUT_RATE__INST_TAG_BF 0.00005
  - MUT_RATE__FUNC_TAG_BF 0.00005
- Outcomes
  - A solution evolved! Large population + low mutation rate helps population maintain high fitness
    individuals (in the face of mutational meltdown on regulatory networks).
  - Global memory solutions & regulation solutions evolve
    - global memory solutions leverage fact that I don't include 0 as an input value

## 2020-06-25 - Explorations

- Changes
  - Seed offset: 7000
  - Guarantee 0 as a possible input value for each computation type
  - Add NUM,NUM,OP1=ERR examples