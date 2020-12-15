#!/bin/bash


# Location of Empirical include directory (relative to scripts directory)
EMP_DIR=../../Empirical/include
EXEC=StreakMatches

g++ src/${EXEC}.cc -o ${EXEC} -I${EMP_DIR} -std=c++17
./${EXEC}