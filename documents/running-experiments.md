# Compile and run experiments locally

Here, we provide a brief guide to compiling and running our experiments.

Please file an [issue](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/issues) if something is unclear or does not work.

## Docker

On docker hub: <https://hub.docker.com/r/amlalejini/tag-based-genetic-regulation-for-gp>

```
docker pull amlalejini/tag-based-genetic-regulation-for-gp
```

This will create a docker image with:

- all the requisite dependencies installed/downloaded
- all experiments compiled
- our raw data downloaded
- a build of our supplemental material (which will also run all of our analyses)

To run the container interactively:

```
docker run -it --entrypoint bash amlalejini/tag-based-genetic-regulation-for-gp
```

You can exit the container at any point with `ctrl-d`.

Inside the container, you should be at `/opt/Tag-based-Genetic-Regulation-for-LinearGP/`.
If you `ls` you should see something like this (maybe not exactly):

```
Dockerfile                                                       documents
Gemfile                                                          experiments
LICENSE                                                          index.Rmd
Makefile                                                         media
README.md                                                        requirements.txt
_bookdown.yml                                                    scripts
_config.yml                                                      source
_output.yml                                                      style.css
alt-signal-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp  supplemental
bool-calc-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp   supplemental.bib
build_book.sh                                                    supplemental_files
build_exps.sh                                                    tail.Rmd
chg-env-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp     tests
```

The important thing is that there should be three executables (with absurdly long names):

- `chg-env-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp`
  - Use this to run the changing-signal problem.
  - To generate a default configuration file, `chg-env-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp --gen`
- `alt-signal-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp`
  - Use this to run the repeated-signal problem.
  - To generate a default configuration file, `alt-signal-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp --gen`
- `bool-calc-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp`
  - Use this to run any of the boolean-logic calculator problems and the contextual-signal problem.
  - To generate a default configuration file, `bool-calc-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp --gen`

Find the exact configurations we used for our experiments here: <https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/tree/master/experiments>

## Manually

This guide assumes an Ubuntu-flavored Linux operating system. These instructions _should_ mostly work for MacOS; otherwise, we recommend using our Docker image or a virtual machine.

Our experiments are implemented in C++, so you'll need a modern C++ compiler capable of compiling C++17 code.

E.g., I'm using:

```
g++ (Ubuntu 10.2.0-13ubuntu1) 10.2.0
```

### Get dependencies

First, make a directory were we can put this project and all of its dependencies.

```
mkdir workspace
cd workspace
```

Next, clone this repository into your new directory.

```
git clone https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP.git
```

Our experiments depend on the Empirical and SignalGP libraries on GitHub.
Inside the `workspace` directory, we'll clone SignalGP and checkout the appropriate commit.

```
git clone https://github.com/amlalejini/SignalGP.git
cd SignalGP
git checkout 83d879cfdb6540862315dc454c1525ccd8054e65
cd ..
```

Next, let's get Empirical (also stick this in the `workspace` directory).

```
git clone https://github.com/amlalejini/Empirical.git
cd Empirical
git checkout e72dae6490dee5caf8e5ec04a634b483d2ad4293
```

We're not quite done with Empirical. We need to grab all of Empirical's dependencies. This will install _all_ of Empirical's dependencies, including those needed to build its documentation/tests.

```
make install-dependencies
```

OR, if you don't want _all_ of that, instead you could do:

```
git submodule init
git submodule update
```

If you don't have libssl-dev, you'll also want to install that (some of the tag-matching metrics use cryptographic hash). E.g.,

```
sudo apt-get install libssl-dev
```

Now we should be good to compile the three executables that we used for our experiments. Inside `workspace/Tag-based-Genetic-Regulation-for-LinearGP/`:

```
./build_exps
```

This script just sets some environment variables (e.g., to define which experiment to compile, the tag-matching metric, etc.) and runs `make native`.

To use a different compiler (than g++), you'll need to change `CXX_nat` in the makefile.

This should create three executables (with absurdly long names):

- `chg-env-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp`
  - Use this to run the changing-signal problem.
  - To generate a default configuration file, `chg-env-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp --gen`
- `alt-signal-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp`
  - Use this to run the repeated-signal problem.
  - To generate a default configuration file, `alt-signal-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp --gen`
- `bool-calc-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp`
  - Use this to run any of the boolean-logic calculator problems and the contextual-signal problem.
  - To generate a default configuration file, `bool-calc-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp --gen`

Find the exact configurations we used for our experiments here: <https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/tree/master/experiments>