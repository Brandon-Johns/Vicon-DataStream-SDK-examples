# CMakeList.txt : Top-level CMake project file
#	Global configuration and include sub-projects
cmake_minimum_required (VERSION 3.10)
project ("VDS_Template_By_Brandon")

# Written by:		Brandon Johns
# Version created:	2021-11-05
# Last edited:		2021-11-05
# Purpose:			Template and Examples to use the Vicon DataStream SDK Version 1.11.0

# Instructions:
#	Check all file paths. They should be specified per their location on your computer
#	e.g. VICONDS_LIB_DIR and VICONDS_INC_DIR


####################################################################################
# Key locations
##########################################
# This CMake project
set(PROJECT_DIR         "${CMAKE_CURRENT_LIST_DIR}/.." CACHE PATH "dir This CMake Project")
set(PROJECT_DIR_RUNTIME "${PROJECT_DIR}/bin"           CACHE PATH "dir to output exe into")


####################################################################################
# General options
##########################################
set(CMAKE_CXX_STANDARD 20)

# Compiler flags (Assumes GCC compiler)
if(NOT WIN32)
	# Add debugging symbols to executable
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

	# This part addresses issues specific to the Vicon DataStream SDK
	#	-Wabi-tag
	#		Enable backwards compatibility warnings
	#	-D_GLIBCXX_USE_CXX11_ABI=0
	#		Please see: https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html
	#		If you have problems, try toggle this between 0 or 1
	#		Example issue: compile error or segfault when building with other libraries
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wabi-tag")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0")
endif()

# For VSCode extention "clangd"
#	CMAKE_EXPORT_COMPILE_COMMANDS generates the "compile_commands.json" file in the build dir
#	Reload the "clangd" extension to make it find the file
#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Path for output exe
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_DIR_RUNTIME}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}") # For Windows
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}") # For Windows

####################################################################################
# Paths to: Libraries, Includes (library header files), Packages
##########################################
# These paths are dependent on the machine you're working on
# Hence, try to identify your current mahcine by checking the hostname and other info
#	Good variables to check: https://cmake.org/cmake/help/latest/command/cmake_host_system_information.html
# Then Setup your machine in an elseif statement

cmake_host_system_information(RESULT HOSTNAME_THIS_PC QUERY HOSTNAME)
message("VDS: hostname for this comuter: ${HOSTNAME_THIS_PC}")

# Windows: All windows installs should look the same
if(WIN32)
	# VDS
	set(VICONDS_LIB_DIR "C:/Program Files/Vicon/DataStream SDK/Win64/CPP" CACHE PATH "dir vicon lib")
	set(VICONDS_INC_DIR "C:/Program Files/Vicon/DataStream SDK/Win64/CPP" CACHE PATH "dir vicon h")

# Linux: All installs should look the same if VDS is installed per the instructions
else
	# VDS
	set(VICONDS_LIB_DIR "${PROJECT_DIR}/lib_ubuntu/vds" CACHE PATH "dir vicon lib")
	set(VICONDS_INC_DIR "${PROJECT_DIR}/lib_ubuntu/vds" CACHE PATH "dir vicon h")
endif()

####################################################################################
# Find all libraries & packages
##########################################
# find_library() assumes that for a library "x"
#	Windows filename: x.lib
#	Windows filename: x.dll (shared library)
#	Linux filename: libx.a
#	Linux filename: libx.so (shared library)

# Find libraries (<varName> <fileName> <filePath> DOC <docstring>)
find_library(VICONDS_LIB "ViconDataStreamSDK_CPP" PATHS ${VICONDS_LIB_DIR} DOC "libViconDataStreamSDK_CPP lib" REQUIRED)

find_package(Threads REQUIRED)
set(THREADS_LIB "Threads::Threads")

####################################################################################
# Projects to build
##########################################
# Include sub-projects (sub-folders with other CMakeList.txt files)

add_subdirectory ("vicon_template")
#add_subdirectory ("my_other_project_folder")


