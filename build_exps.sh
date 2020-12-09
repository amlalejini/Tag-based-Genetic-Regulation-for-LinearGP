#!/bin/bash

# shared compile-time configuration
export MATCH_METRIC=streak
export MATCH_THRESH=0
export MATCH_REG=exp
export TAG_NUM_BITS=256

make clean

# Build changing-signal problem
echo "Compiling changing-signal problem..."
export PROJECT=chg-env-exp
make native
echo "...Done."

echo "Compiling repeated-signal problem..."
# Build repeated-signal problem
export PROJECT=alt-signal-exp
make native
echo "...Done."

echo "Compiling boolean logic calculator problem..."
# Build boolean logic calculator problem
export PROJECT=bool-calc-exp
make native
echo "...Done."

