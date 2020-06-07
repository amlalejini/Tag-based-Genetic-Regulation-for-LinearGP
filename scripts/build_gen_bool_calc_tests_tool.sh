#!/bin/bash
g++-9 GenBoolCalcTestsPostfix.cc -o gen_tests -I ../../Empirical/source -std=c++17
./gen_tests