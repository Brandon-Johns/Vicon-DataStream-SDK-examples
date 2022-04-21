#!/usr/bin/bash
# Written by:		Brandon Johns
# Version created:	2021-11-05
# Last edited:		2021-12-06

# Purpose: Brandon's custom shell commands. Effectively works as if in ~/.bashrc

# To setup
#	Change all instances (in this file only) of "path_to_project_root"
#	to the actual path of what is here called "Template_CPP"

# To activate, add the folowing to ~/.bashrc
#	# Source my settings form another file
#	chmod +x /path_to_project_root/scripts/*
#	source /path_to_project_root/bashrc_append.sh

### NOTES ###
# aliases are garbage (something about them not recursively expanding). Use functions
# Basic function template:
#	myFunction() { myCommand "$@"; }
#		Note: Use exact spacing and terminate with semicolon
#		Note: (not required) The "$@" passes all arg (except $0) from myFunction to myCommand

######################################################
bjScriptsDir='/path_to_project_root/scripts'
bjCMakeDir='/path_to_project_root'

# # Macros - pure
echo 'CUSTOM COMMANDS: bjbuild'
bjbuild() { ${bjScriptsDir}/bj_run_cmake.sh ${bjCMakeDir}; }





