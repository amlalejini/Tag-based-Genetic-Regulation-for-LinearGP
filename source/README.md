# source/

[./native/](./native/) contains the .cc files for our genetic programing problems:

- Repeated-signal problem (alt-signal-exp.cc)
- Contextual-signal problem (bool-calc-exp.cc)
- Changing-signal problem (chg-env-exp.cc)
- Boolean logic calculator problem (bool-calc-exp.cc)

Below, we give the external dependencies for compiling our experiments, and we give which source files are associated with each of the problems.

## External Library Dependencies

- Empirical
  - We used a (minimally) modified version of the Empirical library that can be found on my fork of Empirical; here's the exact commit: <https://github.com/amlalejini/Empirical/tree/e72dae6490dee5caf8e5ec04a634b483d2ad4293>
    - commit `e72dae6490dee5caf8e5ec04a634b483d2ad4293`
  - Main Empirical repository: <https://github.com/devosoft/Empirical>
    - If you're interested in using Empirical in your own projects, I recommend jumping over to the main Empirical repository!
- SignalGP implementation
  - repository: <https://github.com/amlalejini/SignalGP>
  - here's a link to the exact commit: <https://github.com/amlalejini/SignalGP/tree/83d879cfdb6540862315dc454c1525ccd8054e65>
    - commit `83d879cfdb6540862315dc454c1525ccd8054e65`

## Repeated-signal problem

Also called 'alternating signal environment' in source code.

Experiment-specific source files:

- [AltSignalConfig.h](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/AltSignalConfig.h)
- [AltSignalOrg.h](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/AltSignalOrg.h)
- [AltSignalWorld.h](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/AltSignalWorld.h)
- [native/alt-signal-exp.cc](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/native/alt-signal-exp.cc)

## Contextual-signal problem

The contextual-signal problem is (perhaps confusingly) carried out using the boolean logic calculator source code. Except, instead of providing input/output examples that describe calculator problems, we configure categorical output (`CATEGORICAL_OUTPUT`) and provide examples that look more like [this](../experiments/2020-11-27-context-sig/hpcc/examples_S4.csv).

- [BoolCalcConfig.h](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/BoolCalcConfig.h)
- [BoolCalcOrg.h](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/BoolCalcOrg.h)
- [BoolCalcTestCase.h](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/BoolCalcTestCase.h)
- [BoolCalcWorld.h](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/BoolCalcWorld.h)
- [native/bool-calc-exp.cc](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/native/bool-calc-exp.cc)

## Changing-signal problem

Also called 'changing environment problem' in source code.

Experiment-specific source files:

- [ChgEnvOrg.h](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/ChgEnvOrg.h)
- [ChgEnvWorld.h](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/ChgEnvWorld.h)
- [ChgEnvConfig.h](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/ChgEnvConfig.h)
- [native/chg-env-exp.cc](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/native/chg-env-exp.cc)

## Boolean logic calculator problem

- [BoolCalcConfig.h](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/BoolCalcConfig.h)
- [BoolCalcOrg.h](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/BoolCalcOrg.h)
- [BoolCalcTestCase.h](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/BoolCalcTestCase.h)
- [BoolCalcWorld.h](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/BoolCalcWorld.h)
- [native/bool-calc-exp.cc](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/blob/master/source/native/bool-calc-exp.cc)

## Cross-task Utilities

- Event.h
  - Defines custom SignalGP event types used across different tasks.
- matchbin_regulators.h
  - Defines the exponential tag-matching regulator used in this work.
- mutation_utils.h
  - Defines mutator utility for SignalGP programs.
- reg_ko_instr_impls.h
  - Defines knockout regulation instruction implementations.
  - The need for knocking out promotion/repression individually (as opposed to just knocking out regulation
    entirely) came later on during this project. Each regulation instruction has a KO_UP_REG and KO_DOWN_REG
    variant. While less than ideal, this was the least invasive way work this functionality in at the
    time.
