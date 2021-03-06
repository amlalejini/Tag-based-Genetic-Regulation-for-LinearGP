# ALife 2020 - Analysis - Directory Guide

<!-- TOC -->

- [Script descriptions](#script-descriptions)
  - [Data aggregation scripts](#data-aggregation-scripts)
  - [Data processing scripts](#data-processing-scripts)
  - [Data analysis scripts](#data-analysis-scripts)
- [Data pipelines](#data-pipelines)
  - [Repeated signal task](#repeated-signal-task)
  - [Changing signal task](#changing-signal-task)
  - [Directional signal task](#directional-signal-task)
  - [Comparing networks that solve the directional signal task versus the repeated signal task](#comparing-networks-that-solve-the-directional-signal-task-versus-the-repeated-signal-task)
  - [Multicellular signal-differentiation task](#multicellular-signal-differentiation-task)

<!-- /TOC -->

## Script descriptions

This document is as much for future me as it is for you! Please excuse the haste in which many of these scripts were written...

### Data aggregation scripts

Scripts for aggregrating data from completed jobs (submitted using the slurm scripts in [../hpcc/](../hpcc/)):

- aggMaxFitAltSig.py
  - Aggregate solutions/max fitness genotypes (and associated data) for each replicate of the
    repeated signal task.
- aggMaxFitChgEnv.py
  - Aggregate solutions/max fitness genotypes (and associated data) for each replicate of the
    changing signal task.
- aggMaxFitDirSig.py
  - Aggregate solutions/max fitness genotypes (and associated data) for each replicate of the
    directional signal task.
- aggMaxFitMCReg.py
  - Aggregate solutions/max fitness genotypes (and associated data) for each replicate of the
    multicellular differentiation task.

Also note that [requirements.txt](./requirements.txt) gives the python requirements for running the aggregation scripts (namely, hjson).

### Data processing scripts

- genAltSigGraphs.py
  - Generates data files about repeated signal task regulatory networks.
  - Input: regulation files produced by aggMaxFitAltSig.py.
  - Output: regulatory network graph summary + input files for R's igraph package (with --igraphs flag).
- genDirSigGraphs.py
  - Generates data files about directional signal task regulatory networks.
  - Input: regulation files produced by by aggMaxFitDirSig.py.
  - Output: regulatory network graph summary + input files for R's igraph package (with --igraph flag).
- generateMCResponsePatterns.py
  - Generates the final response patterns (as a .csv) of max fitness genotypes extracted from completed evolution runs.
  - Input: output of aggMaxFitMCReg.py
  - Output: data file to be loaded into gen_mc_resp_profiles.R
- gen_mc_resp_profiles.R
  - Generates images depicting the response patterns of evolved genotypes.
  - Input: data file generated by generateMCResponsePatterns.py
  - Output: images!
- gen_reg_profiles_altsig.R
  - Generates visualizations of traces from programs solving the repeated signal task.
  - Input: Trace files from aggMaxAltSig.py
  - Output: images!
- gen_reg_profiles_dirsig.R
  - Generates visualizations of traces from programs solving the directional signal task.
  - Input: Traces files from aggMaxFitDirSig.py

### Data analysis scripts

- changing-signal-task-analysis.Rmd.Rmd
  - Data analyses for changing signal task.
- directional-signal-task-analysis.Rmd
  - Data analyses for directional signal task.
- repeated-signal-task-analysis.Rmd
  - Data analyses for the repeated signal task.
- regulatory-network-comparison.Rmd
  - Data analyses for comparing regulatory networks evolved in the repeated signal and directional
    signal tasks.
- multicell-differentiation-task.Rmd
  - Data analyses for the multicellular differentiation task.
  - Note that these analyses were cut from the final paper, so, for the sake of space, we do not include
    the data necessary to run these analyses.


## Data pipelines

### Repeated signal task

- Trace visualizations
  - Experiment output (output of experiment implemented in C++) => aggMaxFitAltSig.py
    - => gen_reg_profiles_altsig.R
    - => repeatedSignalTaskAnalysis.Rmd
- Final data analyses
  - Experiment output => aggMaxFitAltSig.py => genAltSigGraphs.py => repeated-signal-task-analysis.Rmd

### Changing signal task

- Final data analyses
  - Experiment output => aggMaxFitChgEnv.py => changing-signal-task-analysis.Rmd.Rmd

### Directional signal task

- Trace visualizations
  - Experiment output (output of experiment implemented in C++) => aggMaxFitDirSig.py
    - => gen_reg_profiles_dirsig.R
    - => directional-signal-task-analysis.Rmd
- Final data analyses
  - Experiment output => aggMaxFitDirSig.py => genDirSigGraphs.py => directional-signal-task-analysis.Rmd

### Comparing networks that solve the directional signal task versus the repeated signal task

- Final data analyses
  - Experiment output => [aggMaxFitDirSig.py, aggMaxFitAltSig.py] => regulatory-network-comparison.Rmd

### Multicellular signal-differentiation task

- Reponse visualizations
  - Experiment output => aggMaxFitMCReg.py => generateMCResponsePatterns.py => gen_mc_resp_profiles.R
- Data analyses
  - Experiment output => aggMaxFitMCReg.py => multicell-differentiation-task-analysis.Rmd