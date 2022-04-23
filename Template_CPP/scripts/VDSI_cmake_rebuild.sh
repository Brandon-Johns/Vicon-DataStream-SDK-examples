##!/bin/bash
## Written by:		Brandon Johns
## Version created:	2022-03-15
## Last edited:		2022-03-15

## Purpose: Destroy old build and rebuild with cmake

## Error action (exit!=0 or unbound var) -> terminate script
set -euo pipefail

####################################################################################
## Automated - Read Input
##########################################
## (Required) Path to this CMake project
Project_Root=$1

## Validate: Require input
## Validate: Location of CMakeLists.txt
if [ "$Project_Root" = "" ] || [[ ! -f "${Project_Root}/src/CMakeLists.txt" ]]
then
  echo "VDSI_ERROR @ $0"
  echo "VDSI_ERROR: invalid arg(1) the project directory"
  exit 1
fi

################################################################
# Automated - Main Section
################################
Project_src="${Project_Root}/src" ## Path to my source
Project_build="${Project_Root}/build" ## Path to cache

## Create build directory
if [[ ! -d "$Project_build" ]]; then mkdir -p "$Project_build"; fi

## Validate location of CMakeCache.txt (because running delete is dangerous)
## Then Empty /build (but don't delete the dir itself)
if [[ -f "${Project_build}/CMakeCache.txt" ]]; then find "${Project_build}" -mindepth 1 -delete ; fi

## Generate cache
cmake -S "${Project_src}" -B "${Project_build}"

## Build my code
cd "${Project_build}"
cmake --build "${Project_build}"

