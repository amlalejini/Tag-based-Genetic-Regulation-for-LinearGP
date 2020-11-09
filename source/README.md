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
  - repository: <https://github.com/devosoft/Empirical>
  - commit: `d4306d33d5e68a3a52b304bd4b1ea394958dd8bd`
- SignalGP implementation
  - repository: <https://github.com/amlalejini/SignalGP>
  - commit: `63d1966ca7fa98d3de634e5e8c316099ab9f68be`

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
