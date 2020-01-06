# Experiment - Demonstrating Function Regulation in SignalGP

Hypothesis:

- Regulation is valuable in alternativing signal environment and multi-cell development environment. Regulation is not necessary (but not harmful) in simple changing environment.
- Global memory is a control.

Environments:

- Alternating signal environment
  - **Environment**: N environments, presented in consistent order (env-1, env-2, env-3, ...) every evaluation. Identical signal given at beginning of each environment. Organisms must regulate reponse to signal (i.e., env-1 requires response-1, env-2 requires response-2). Can achieve this plasticity via memory or function regulation.
  - **Hypothesis**: regulation valuable
  - **Experiment**:
    - Parameters:
      - replicates - 50
      - regulation - multiplicative
      - matching - streak
      - tag width (2) - 64, 128 (limited cross section)
      - selector - ranked
      - Mutation rates - (same as reg-type-exps)
      - NUM_SIGNAL_RESPONSES (num envs) - 2, 4, 8, 16, 32
      - NUM_ENV_CYCLES = NUM_SIGNAL_RESPONSES
      - Organism Functionality - Regulation only, Memory only, Memory + Regulation, No memory + No regulation
- Multi-cell regulation environment
  - **Environment**: Organisms are multicellular. Organisms begin as a single cell and must develop (via internal cell-replication). Evaluation occurs in two phases: development phase and a signal-response phase. During development, cells are allowed to self-replicate ('tissue accretion'). During the response-phase, cells must respond to an exogenous signal. To maximize multi-cell fitness, cells should (evenly) differentiate their responses.
  - **Hypothesis**: regulation valuable, epigenetics valuable for differentiation
  - **Experiment**:
    - Parameters:
      - replicates - 50
      - regulation - multiplicative
      - matching - streak
      - tag width (2) - 64
      - selector - ranked
      - Mutation rates - (same as reg-type-exps)
      - NUM_SIGNAL_RESPONSES (num envs) - 2, 4, 8, 16, 32
      - Organism Functionality - Regulation only, Memory only, Memory + Regulation, No memory + No regulation
      - Epigenetics - Regulation only + Epigenetics (does epigenetics help?)
- Simple changing environment
  - **Environment**: N environments, each with a unique signal. Maximize fitness by responding appropriately to each environment.
  - **Hypothesis**: regulation unnecessary (but not harmful)
  - **Experiment**:
    - Parameters:
      - replicates - 50
      - regulation - multiplicative
      - matching - streak
      - tag width (2) - 64
      - selector - ranked
      - Mutation rates - (same as reg-type-exps)
      - NUM_SIGNAL_RESPONSES (num envs) - 2, 4, 8, 16, 32
      - Organism Functionality - Regulation only, Memory only, Memory + Regulation, No memory + No regulation
