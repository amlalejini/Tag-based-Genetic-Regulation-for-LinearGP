# Genetic Regulation Facilitates the Evolution of Signal-response Plasticity in Digital Organisms

**Navigation**

<!-- TOC -->

- [Overview](#overview)
  - [Abstract](#abstract)
  - [SignalGP](#signalgp)
  - [Genetic Regulation in SignalGP](#genetic-regulation-in-signalgp)
  - [Contributing authors](#contributing-authors)
- [Reproducibility](#reproducibility)
- [Supplemental Material](#supplemental-material)
- [Repository Guide](#repository-guide)
- [References](#references)

<!-- /TOC -->

## Overview

This repository is a companion to our 2020 ALife conference submission, Genetic Regulation Facilitates
the Evolution of Signal-response Plasticity in Digital Organisms.

### Abstract

> In digital evolution, self-replicating computer programs (digital organisms) mutate, compete, and evolve _in silico_.
  Traditional forms of digital organisms execute procedurally, starting at the top of their program (genome) and proceeding in sequence.
  SignalGP (Signal-driven Genetic Programs) allows digital organisms to automatically respond to signals from the environment or other agents in a biologically-inspired manner.
  We augment SignalGP with genetic regulation, allowing organisms to dynamically adjust their response to signals during their lifetime.
  We demonstrate that this capacity for arbitrary gene regulatory networks facilitates the evolution of signal-response plasticity in digital organisms on simple diagnostic tasks.
  We find that more challenging diagnostic environments select for larger, more interconnected regulatory networks.
  We also observe that when signal-response plasticity is not required, erroneous regulation can manifest as maladaptive cryptic variation, impeding task generalization.
  In addition to exploring the effects of regulation on well-understood tasks, we evaluate whether digital organisms evolve to functionally incorporate genetic regulation in a less-prescribed domain where
  fraternal transitions in individuality can evolve _de novo_.
  We identify several multicellular lineages that adaptively employ genetic regulation and demonstrate that genetic regulation can enable evolved mechanisms for mutual exclusion.
  As SignalGP digital organisms become more lifelike, we broaden digital evolution's repertoire of experimental possibilities, allowing us to engage in more realistic studies of evolutionary dynamics.

### SignalGP

![sgp-cartoon](./media/sgp-cartoon.svg)

SignalGP is a genetic program representation that allows digital organisms to dynamically react to
signals from the environment or from other agents.
In traditional digital organisms, programs comprise linear instruction sequences that are executed
procedurally: instructions are processed one at a time in a single chain of execution, and instructions
must explicitly check for new sensory information.
As such, these forms of traditional digital organisms must generate explicit queries to identify (and
respond to) changes in their environment.

In SignalGP, program expression is signal-driven.
Programs are segmented into genetic modules (or functions), and each module can be independently triggered
in response to a signal.
Each module associates a tag with a linear sequence of instructions.
Tags are labels that can mutate and evolve, and the similarity (or dissimilarity) between any two tags
can be computed.
In this work, tags are represented as fixed-length bit strings, and we compute the similarity, or match-score,
between any two tags using the streak metric (Downing, 2015).

Signals can originate exogenously (e.g, from the environment or other agents) or endogenously (e.g., self-signaling).

For a more detailed description of the SignalGP representation (albeit in more of a evolutionary computation/
genetic programming vein), see [(Lalejini and Ofria, 2018)](https://doi.org/10.1145/3205455.3205523).

### Genetic Regulation in SignalGP

![regulation-example](./media/regulation-example-cartoon.svg)

### Contributing authors

## Reproducibility

## Supplemental Material

## Repository Guide

## References

Downing, K. L. (2015). Intelligence emerging: Adaptivity and search in evolving neural systems. The MIT Press.

Lalejini, A., & Ofria, C. (2018). Evolving event-driven programs with SignalGP. Proceedings of the Genetic and Evolutionary Computation Conference on   - GECCO ’18, 1135–1142. https://doi.org/10.1145/3205455.3205523
