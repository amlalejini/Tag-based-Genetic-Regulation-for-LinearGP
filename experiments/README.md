# Experiment Log

## Mutation rate experiments (11/26ish)

Look for reasonable mutation rate + match metric to use going forward.

[See experiment readme for details.](./mut-rate-exps/)

Post-experiment, major changes to matchbin regulator. Need to re-write regulation instructions & run
add vs multiplicative regulator experiment (to pick one to use going forward).

## Regulation type experiments (12/17)

Look at multiplicative vs additive module regulation to decide which to use going forward.

[See experiment readme for more details.](./reg-type-exps/)

## Demonstrating regulation experiment

Goal: Demonstrate function regulation in SignalGP

[See experiment readme for more details.](./dem-reg-exps/)

Things to try

- Alternating signal
  - 16 and 32
    - 128 tag width
    - 128 functions
- Multicell regulation
  - [x] 20x20, 10 roles, less development time => Running (1/13)
  - Reward 'clumpy-ness'

## Exploring tag-matching metrics

[Here (./tag-metric-exps/)](./tag-metric-exps/)

## ALife 2020 - SignalGP Genetic Regulation

Repeated Signal Environment (AltSignalWorld)

- Treatments:
  - Regulation
  - Global Memory
  - Regulation + Global Memory
  - None
- General Parameters
  - 10K generations, snapshot every 1k (look early and late)
  - Environment states: 2, 4, 8, 16
- Questions:
  - Does regulation enable evolution of successful programs in repeated signal environment?
  - Is regulation important component of evolved successful strategies?
    - Use knockout fitness differentials!
  - Is memory an important component of evolved successful strategies?
  - CASE STUDY: trace solution that uses regulation to illustrate exactly how regulation is used in
    controlling the organism's responses.
- Secondary experiment
  - Regulation + 128 tag width
  - Questions
    - Is tag width the limiting factor for evolving solutions to many-state environments?

Simple Changing Environment (ChgEnvWorld)

- Treatments:
  - Regulation
  - Global Memory
  - Regulation + Global Memory
  - None
- General parameters
  - 10K generations, snapshot every 1k (look early and late)
  - Environment states: 2, 4, 8, 16
- Hypothesis
  - Regulation not necessary here. Solutions will evolve in all conditions, regulation should not have
    negative impact on evolution of solutions.

Multicell Development (MCRegWorld)

- (primary) Mechanisms for differentiating
  - Messaging, imprinting, reproduction signaling
- Treatments
  - Regulation (+all modes of differentiation)
  - Global memory
  - Regulation & global memory
  - None
- General parameters
  - 1k generations (these runs can be slow)
  - Demes: 10x10x10, 20x20x20
- Secondary experiments (30 reps each)
  - Regulation + messaging
  - Regulation + imprinting
  - Regulation + offspring signaling
  - Regulation + messaging, imprinting, offspring signaling
  - Regulation + none
