#!/bin/bash

# shared compile-time configuration
export MATCH_METRIC=streak
export MATCH_THRESH=0
export MATCH_REG=exp
export TAG_NUM_BITS=256

make clean

# Build independent-signal problem
echo "Compiling independent-signal problem..."
export PROJECT=chg-env-exp
make native
echo "...Done."

echo "Compiling signal-counting problem..."
# Build signal-counting problem
export PROJECT=alt-signal-exp
make native
echo "...Done."

echo "Compiling boolean logic calculator problem..."
# Build boolean logic calculator problem
export PROJECT=bool-calc-exp
make native
echo "...Done."
