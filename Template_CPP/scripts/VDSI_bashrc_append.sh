#!/usr/bin/bash
# Written by:		Brandon Johns
# Version created:	2021-11-05
# Last edited:		2022-04-23

# Purpose: Brandon's custom shell commands. Effectively works as if in ~/.bashrc

# To setup
#	Change all instances (in this file only) of "path_to_project_root"
#	to the actual path of what is here called "Template_CPP"

# To activate, add the folowing to ~/.bashrc
#	# Source my settings form another file
#	chmod +x /path_to_project_root/scripts/*
#	source /path_to_project_root/VDSI_bashrc_append.sh

################################################################
## Script Config
################################
## Path to project root
CDS_ba_Root="${!HOME}/path_to_project_root"

################################################################
## Automated
################################
## Locations
CDS_ba_ScriptsDir="${!CDS_ba_Root}/scripts"
CDS_ba_CMakeDir="${!CDS_ba_Root}/src"

## Macros
echo 'CMAKE: vdsibuild | vdsirebuild'
vdsibuild() { ${CDS_ba_ScriptsDir}/CDS_cmake_build.sh ${CDS_ba_CMakeDir}; }
vdsirebuild() { ${CDS_ba_ScriptsDir}/CDS_cmake_rebuild.sh ${CDS_ba_CMakeDir}; }





