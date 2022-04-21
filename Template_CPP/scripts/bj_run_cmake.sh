#!/bin/bash
# Written by:		Brandon Johns
# Version created:	2021-10-05
# Last edited:		2021-10-05

# Purpose: Destroy old build and rebuild with cmake

# Require input
# Basic validation - check for correct relative location of CMake file
PROJECT_DIR=$1
if [ "$1" = "" ] || [[ ! -f "$1/src/CMakeLists.txt" ]]
then
  echo "ERROR @ $0"
  echo "ERROR: invalid arg(1) the project directory"
  exit
fi

cd $PROJECT_DIR
#rm -r build

mkdir build
cd build
cmake ../src
make
