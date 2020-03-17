# Running experiments locally

Here, we provide a brief guide to compiling and running our experiments locally.

Please file an [issue](https://github.com/amlalejini/ALife-2020--SignalGP-Genetic-Regulation/issues) if something is unclear or does not work.

## Dependencies

The diagnostic tasks are implemented in C++. You'll need a C++ compiler capable of compiling C++ 17.

On my local machine (used for development), I'm compiling with Homebrew GCC:

```
g++-9 (Homebrew GCC 9.2.0) 9.2.0
Copyright (C) 2019 Free Software Foundation, Inc.
```

On the computing cluster used to run our experiments:

```
g++ (GCC) 9.1.0
Copyright (C) 2019 Free Software Foundation, Inc.
```

You'll also need the following software:

- Custom fork of the Empirical C++ library: [https://github.com/amlalejini/Empirical/tree/v1.0-ALife2020](https://github.com/amlalejini/Empirical/tree/v1.0-ALife2020)
  - This links to a custom branch of my (amlalejini) fork of the Empirical library at the commit with
    which we ran the diagnostic task experiments (i.e., the software in this repository).
- External implementation of SignalGP: [https://github.com/amlalejini/SignalGP/tree/1.0](https://github.com/amlalejini/SignalGP/tree/1.0)
  - This links to the commit with which we ran the diagnostic task experiments. It's likely the SignalGP
    repository will change over time and become out-of-sync with what is required by the diagnostic
    task experiments implemented here.
- OpenSSL
  - The cryptographic hash tag match metric in Empirical requires OpenSSL.

The includes for each diagnostic task implementation assumes that the paths to Empirical, SignalGP,
and this repository's source directory each have been added as includes (`-I` flag).

## Compiling our experiments locally

You should be able use our [Makefile](./Makefile) with some modifications:

- Set the `PROJECT` variable to the diagnostic you want to compile
  - `PROJECT := alt-signal-exp` for the repeated signal task
  - `PROJECT := chg-env-exp` for the changing signal task
  - `PROJECT := dir-signal-exp` for the directional signal task
- Configure your dependency paths
  - Variables: `EMP_DIR`, `SGP_DIR`, and `OPEN_SSL_DIR`
  - Below is my local configuration:
```
# Dependency directories
EMP_DIR := ../Empirical/source
SGP_DIR := ../SignalGP/source
OPEN_SSL_DIR := /usr/local/Cellar/openssl/1.0.2t
```
- Set `CXX_nat` to the correct compiler
  - e.g., on my local machine this is `CXX_nat := g++-9`
- Set a few compile-time arguments for the experiments
  - `MATCH_METRIC` sets the tag-matching metric for SignalGP to use (these experiments use `MATCH_METRIC := streak`)
  - `MATCH_THRESH` sets the minimum tag-matching score for a successful match (these experiments use `MATCH_THRESH := 25`)
  - `MATCH_REG` sets the tag-matching regulator to use (these experiments use `MATCH_REG := mult`)
  - `TAG_NUM_BITS` sets the number of bits in tag bit strings (these experiments use `MATCH_REG := 64`)

Now, `make debug` should compile in debug mode (will be much slower but have error checking) or use `make native` to compile in release mode (much faster but no error checking).

Note: ignore all of the web-related stuff in the Makefile. As of now, we have not implemented web-versions of our diagnostic experiments.

## Running experiments

Find the configuration details for the diagnostic tasks in our [task descriptions documentation](./task-descriptions.md).

To generate a configuration file (for any of the diagnostic tasks), run the executable with the `--gen` command line argument.
Any run-time configuration parameter can be set either in the `config.cfg` or set on the command line. For example, to set the `SEED` configuration parameter (random number seed) to 10 on the command line, add the argument `-SEED 10`.

When running the experiment executable, it uses parameter values in the following priority order (higher overrides lower):

1. command line arguments of the form `-PARAMETER VALUE`
2. parameter values set in the local `config.cfg` file (in the execution directory)
3. default parameter values specified in the appropriate config header
  - e.g., source/AltSigConfig.h, source/ChgEnvConfig.h, source/DirSignalConfig.h