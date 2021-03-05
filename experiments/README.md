# Experiment log

This directory contains files related to exploratory/preliminary experiments and to the final set of
experiments for our ALife 2020 contribution.

From software development to publication, the names of things sometimes change (often to make the final publication more readable). Note the following experiment name changes:
-  The signal-counting problem is sometimes referred to as the 'repeated-signal problem' or the 'alternating-signal problem'.
- The independent-signal problem is sometimes referred to as the 'changing-signal problem' or the 'changing-environment problem'.

This directory contains job submission/analysis/data associated with our experiments.
See the log below for more information on each experiment.

- [2020-11-11 - Independent-signal problem](#2020-11-11---independent-signal-problem)
- [2020-11-25 - Signal-counting problem](#2020-11-25---signal-counting-problem)
- [2020-11-27 - Contextual-signal problem](#2020-11-27---contextual-signal-problem)
- [2020-11-28 - Boolean-logic calculator problem (prefix notation)](#2020-11-28---boolean-logic-calculator-problem-prefix-notation)
- [2020-11-28 - Boolean-logic calculator problem (postfix notation)](#2020-11-28---boolean-logic-calculator-problem-postfix-notation)
- [ALife 2020 experiments](#alife-2020-experiments)
- [Exploratory experiments](#exploratory-experiments)

## 2020-11-11 - Independent-signal problem

Final changing-signal problem experiment.

[./2020-11-11-chg-sig/](./2020-11-11-chg-sig/)

## 2020-11-25 - Signal-counting problem

Final signal-counting problem experiment.

[./2020-11-25-rep-sig/](./2020-11-25-rep-sig/)

## 2020-11-27 - Contextual-signal problem

Final contextual-signal problem experiment.

[./2020-11-27-context-sig/](./2020-11-27-context-sig/)

## 2020-11-28 - Boolean-logic calculator problem (prefix notation)

Final prefix notation Boolean-logic calculator problem experiment.

[2020-11-28-bool-calc-prefix](2020-11-28-bool-calc-prefix)

## 2020-11-28 - Boolean-logic calculator problem (postfix notation)

Final postfix notation Boolean-logic calculator problem experiment.

[2020-11-28-bool-calc-postfix](2020-11-28-bool-calc-postfix)

## ALife 2020 experiments

We submitted an early version of this work to the 2020 Artificial Life conference.
The repository associated with our ALife submission can be found here: <https://github.com/amlalejini/ALife-2020--SignalGP-Genetic-Regulation>

[See experiment readme for more details.](./alife-2020/)

## Exploratory experiments

We carried out a series of exploratory experiments for proof of concept and parameter tuning.

- [Mutation rate tuning](./exploratory/mut-rate-exps/)
- [2020-06-04 - bool-calc](./exploratory/2020-06-04-bool-calc/)
  - Early version of boolean calculator problem.
  - Uses postfix notation and requires programs to identify invalid input sequences.
- [2020-08-04 - sig-reprog](./exploratory/2020-08-04-sig-reprog/)
  - Early version of the contextual-signal problem
- [2020-08-05 - bc-prefix](./exploratory/2020-08-05-bc-prefix/)
  - First exploration of prefix notation with boolean-logic calculator problem.
- [2020-11-10 - rep-sig](./exploratory/2020-11-10-rep-sig/)
  - Identical to 2020-11-25 experiment, except did not include instruction execution traces.
- [2020-11-12 - context-sig](./exploratory/2020-11-12-context-sig/)
  - Identical to 2020-11-27 experiment, except did not include instruction execution traces.
- [2020-11-13 - bool-calc-postfix](./exploratory/2020-11-13-bool-calc-postfix/)
  - Identical to 2020-11-28 postfix experiment, except did not include instruction execution traces.
- [2020-11-13 - bool-calc-prefix](./exploratory/2020-11-13-bool-calc-prefix/)
  - Identical to 2020-11-28 prefix experiment, except did nt include instruction execution traces.
