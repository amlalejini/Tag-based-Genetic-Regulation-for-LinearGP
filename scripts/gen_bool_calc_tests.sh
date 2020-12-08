#!/bin/bash


# Location of Empirical include directory (relative to scripts directory)
EMP_DIR=../../Empirical/include

if [[ $1 == "postfix" ]]; then
  echo "Generating postfix notation examples..."
  EXEC=GenBoolCalcTestsPostfix
elif [[ $1 == "prefix" ]]; then
  echo "Generating prefix notation examples..."
  EXEC=GenBoolCalcTestsPrefix
elif [[ $1 == "infix"  ]]; then
  echo "Generating infix notation examples..."
  EXEC=GenBoolCalcTestsInfix
else
  echo "Usage:"
  echo "$ gen_bool_calc_tests.sh NOTATION"
  echo "where NOTATION can be one of three options: 'postfix', 'prefix', or 'infix'"
  exit
fi

cd src
g++ ${EXEC}.cc -o gen -I../${EMP_DIR} -std=c++17
./gen
rm gen
cd ../
mv src/testing_set_$1.csv ./
mv src/training_set_$1.csv ./

echo "Done!"
