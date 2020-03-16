# Diagnostic Tasks

Here, we give a brief description of each of the diagnostic tasks used in this work.

**Navigation**
<!-- TOC -->

- [Repeated Signal Task](#repeated-signal-task)
  - [Experimental configuration](#experimental-configuration)
    - [General parameters](#general-parameters)
    - [Treatment-specific parameters](#treatment-specific-parameters)
- [Directional Signal Task](#directional-signal-task)
  - [Experimental configuration](#experimental-configuration-1)
- [Changing Signal Task](#changing-signal-task)
  - [Experimental configuration](#experimental-configuration-2)
- [Multicellular Signal-differentiation Task](#multicellular-signal-differentiation-task)

<!-- /TOC -->

## Repeated Signal Task

The repeated signal task is one of several diagnostics we used to evaluate whether genetic regulation faculties contribute to, and potentially detract from, the functionality of evolved SignalGP digital organisms.
The repeated signal task requires organisms to exhibit signal-response plasticity; that is, they must shift their response to a repeated environmental signal during their lifetime.

The repeated signal task requires organisms to express the appropriate (distinct) response to a single environmental signal each of the _K_ times that it is repeated.
Organisms express responses by executing one of _K_ response instructions.
For example, if organisms receive four signals from the environment (i.e., _K_=4), a maximally fit organism will express `Response-1` after the first signal, `Response-2` after the second, `Response-3` after the third, and `Response-4` after the fourth.
The figure below depicts examples of optimal behavior in the repeated two- and four-signal tasks.

![repeated-signal-task-overview](./media/repeated-signal-task.svg)

Requiring organisms to execute a distinct instruction for each repetition of the environmental signal represents organisms having to perform distinct behaviors.
Note that each repetition of the environmental signal is identical (i.e., each has an identical tag), and as such, organisms must track how many times the signal has been repeated and dynamically shift their responses accordingly.

We allowed organisms `r time_steps` time steps to express the appropriate response after receiving an environmental signal.
Once the allotted time to respond expires or the organism expresses any response, the organism's threads of execution are reset, resulting in a loss of all thread-local memory.
_Only_ the contents of an organism's global memory and each function's regulatory state persist. The environment then produces the next signal (identical to all previous environment signals) to which the organism may respond.
An organism must use their global memory buffer or genetic regulation to correctly shift its response to each subsequent environmental signal.
An organism's fitness is equal to the number of correct responses expressed during evaluation.

For each number of repeated signals ($K=$ `r env_complexities`), we ran `r replicates` replicate populations (each with a distinct random number seed) of four experimental conditions:

1. a memory-only treatment where organisms must use global memory (in combination with procedural flow-control mechanisms) to correctly respond to environmental signals.
2. a regulation-augmented treatment where organisms have access to both global memory and genetic regulation.
3. a regulation-only control where organisms can adjust signal-responses through regulation.
4. a control where organisms can use neither global memory nor genetic regulation (which should make the repeated signal task impossible to solve; if we see solutions, something is not working as we expected...)

### Experimental configuration

Run-time parameters for the repeated signal task can be found in [./source/AltSignalConfig.h](./source/AltSignalConfig.h).
Generate a configuration file by executing the the repeated signal task executable with the `--gen`
command line option.

#### General parameters

Parameters used across conditions.

| parameter | value | description |
| --- | --- | --- |
| matchbin_metric | streak | Which tag-matching metric should be used for tag-based referencing?  |
| matchbin_thresh | 25 | What is the minimum similarity threshold (as a percentage; e.g., 25 = 25% match) for tag-matches to succeed? |
| matchbin_regulator | mult | Which tag-matching regulator should be used for genetic regulation? |
| TAG_LEN | 64 | Compile-time parameter. Number of bits in each tag bit string. |
| INST_TAG_CNT | 1 | How many argument-tags does each instruction have? |
| INST_ARG_CNT | 3 | How many numeric arguments does each instruction have? |
| FUNC_NUM_TAGS | 1 | How many function-tags does each function have? |
| environment_signal_tag | varied by replicate (randomly generated at the beginning of each run) | |
| SEED | varied by replicate | Random number generator seed |
| GENERATIONS | 10000 | How many generations should the population evolve? |
| POP_SIZE | 1000 | How many individuals are in our population? |
| NUM_ENV_CYCLES | 2, 4, 8, or 16 | How many times are signals repeated? |
| NUM_SIGNAL_RESPONSES | 2, 4, 8, or 16 | How many possible responses are there? |
| CPU_TIME_PER_ENV_CYCLE | 128 | How many time steps do programs have to respond after receiving an environmental signal? |
| MAX_ACTIVE_THREAD_CNT | 8 | How many threads are allowed to be running concurrently? |
| MAX_THREAD_CAPACITY | 16 | How many threads are allowed to be running concurrently + the number allowed to be in a pending state? |
| TOURNAMENT_SIZE | 8 | How many individuals participate in each tournament for parent selection? |
| MIN_FUNC_CNT | 0 | What is the minimum number of functions allowed in a program (constrains the mutation operators)? |
| MAX_FUNC_CNT | 128 | What is the maximum number of functions allowed in a program (constrains the mutation operators)? |
| MIN_FUNC_INST_CNT | 0 | What is the minimum number of instructions allowed in each program function? |
| MAX_FUNC_INST_CNT | 64 | What is the maximum number of instructions allowed in each program function? |
| INST_MIN_ARG_VAL | -8 | What is the minimum value for numeric instruction arguments? |
| INST_MAX_ARG_VAL | 8 | What is the maximum value for numeric instruction arguments? |
| MUT_RATE__FUNC_TAG_BF | 0.0005 | The per-bit mutation rate for function tags |
| MUT_RATE__INST_TAG_BF | 0.0005 | The per-bit mutation rate for instruction-argument tags |
| MUT_RATE__FUNC_DUP | 0.05 | The per-function mutation rate for whole-function duplications |
| MUT_RATE__FUNC_DEL | 0.05 | The per-function mutation rate for whole-function deletions |
| MUT_RATE__SEQ_SLIP | 0.05 | The per-function mutation rate for slip mutations (which can result in instruction-sequence duplications and deletions) |
| MUT_RATE__INST_INS | 0.005 | The per-instruction mutation rate for single-instruction insertions |
| MUT_RATE__INST_DEL | 0.005 | The per-instruction mutation rate for single-instruction deletions |
| MUT_RATE__INST_SUB | 0.005 | The per-instruction mutation rate for substitutions |
| MUT_RATE__INST_ARG_SUB | 0.005 | The per-instruction-argument rate for numeric argument substitutions |
| OUTPUT_DIR | output | What directory will the experiment write output? |
| STOP_ON_SOLUTION | 1 | Should we stop running (and output) as soon as a solution is found? |
| SNAPSHOT_RESOLUTION | 1000 | At what resolution (in generations) should we output 'snapshot' data? |
| SUMMARY_RESOLUTION | 100 | At what resolution (in generations) should we output summary data? |

#### Treatment-specific parameters

Parameters specific to particular experimental treatments.

**Regulation-augmented**

| parameter | value | description |
| --- | --- | --- |
| USE_GLOBAL_MEMORY | 1 | Do SignalGP digital organisms have access to global memory? |
| USE_FUNC_REGULATION | 1 | Do SignalGP digital organisms have access to genetic regulation? |

**Global-memory-only**

| parameter | value | description |
| --- | --- | --- |
| USE_GLOBAL_MEMORY | 1 | Do SignalGP digital organisms have access to global memory? |
| USE_FUNC_REGULATION | 0 | Do SignalGP digital organisms have access to genetic regulation? |

**Regulation-only**

| parameter | value | description |
| --- | --- | --- |
| USE_GLOBAL_MEMORY | 0 | Do SignalGP digital organisms have access to global memory? |
| USE_FUNC_REGULATION | 1 | Do SignalGP digital organisms have access to genetic regulation? |

**Neither global memory nor regulation**

| parameter | value | description |
| --- | --- | --- |
| USE_GLOBAL_MEMORY | 0 | Do SignalGP digital organisms have access to global memory? |
| USE_FUNC_REGULATION | 0 | Do SignalGP digital organisms have access to genetic regulation? |

## Directional Signal Task

### Experimental configuration

TODO - run settings from hpcc

## Changing Signal Task

![changing-signal-task-overview](./media/changing-signal-task.svg)

### Experimental configuration

TODO - grab run settings from the HPCC

## Multicellular Signal-differentiation Task

Note...

![multicell-signal-differentiation-task-overview](./media/multicell-differentiation-task.svg)
