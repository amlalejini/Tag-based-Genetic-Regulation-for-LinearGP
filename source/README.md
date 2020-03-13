# Source

[./native/](./native/) contains the .cc files for each of the four experiments:

- Repeated signal task
- Changing signal task
- Directional signal task
- Multicellular differentiation task

Note that that the 'repeated signal task' is also referred to as the 'alternating signal environment',
the 'changing signal task' is also referred to as the 'changing environment', and the 'multicellular
differentiation task' is also referred to as the 'multicellular regulation task/environment'.

When we down-scoped the paper, we excluded discussion (and further experimentation with) the multicellular
differentiation task.

The [./web/](./web/) is where we would house the .cc files for Emscripten web versions. As of me writing
this readme, web versions of experiments do not exist, and as such, the web directory has only a stub
.cc file.

## External Library Dependencies

- Empirical
- SignalGP implementation
  - We use a new implementation

## Repeated Signal Task

Also called 'alternating signal environment'.

Experiment-specific source files:

- AltSignalConfig.h
- AltSignalOrg.h
- AltSignalWorld.h

## Changing Signal Task

Also called 'changing environment'.

Experiment-specific source files:

- ChgEnvOrg.h
- ChgEnvWorld.h
- ChgEnvConfig.h


## Directional Signal Task

Experiment-specific source files:

- DirSignalWorld.h
- DirSignalConfig.h
- DirSignalOrg.h

## Multicellular Differentiation Task

Also called 'multicellular regulation environment'.

Experiment-specific source files:

- MCRegWorld.h
- MCRegConfig.h
- MCRegDeme.h
- MCRegOrg.h

As mentioned above, this task was cut from the ALife 2020 contribution when we scoped-down the paper.
We used this task to do initial explorations into the effects/value of epigenetic inheritance of regulatory
elements in a simple multicellular differentiation task. In brief, genetically homogeneous groups of
SignalGP cells were evaluated as individuals, and fitness was assigned based on how well cells differentiated
their response to an environmental signal and, optionally, how _clustered_ responses were.

Because this experiment was cut from the paper, the task's implementation lags the other tasks' implementations.

## Cross-task Utilities

- Event.h
  - Defines custom SignalGP message types used across different tasks.
- mutation_utils.h
  - Defines mutator utility for SignalGP programs.
- reg_ko_instr_impls.h
  - Defines knockout regulation instruction implementations.
  - The need for knocking out promotion/repression individually (as opposed to just knocking out regulation
    entirely) came later on during this project. Each regulation instruction has a KO_UP_REG and KO_DOWN_REG
    variant. While less than ideal, this was the least invasive way work this functionality in at the
    time.
