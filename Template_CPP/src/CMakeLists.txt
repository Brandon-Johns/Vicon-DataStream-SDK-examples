# CMakeList.txt : Top-level CMake project file
#	Global configuration and include sub-projects
cmake_minimum_required (VERSION 3.10)
project ("VDS_Template_By_Brandon")

# Written by:		Brandon Johns
# Version created:	2021-11-05
# Last edited:		2021-11-05

####################################################################################
# General options
##########################################
# Use C++20
set(CMAKE_CXX_STANDARD 20)

# Compiler flags
# Could split between debug and release versions... not nessesary rn
#	More compiler warnings: -Wall -Wextra
#	Add debugging symbols to executable: -g
#	Opimise: -O3
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# This part is hell
#	Enable backwards compatibility warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wabi-tag")
#	Enable backwards compatibility mode
#	Try toggle this to "=1" if you have problems
#	If you're still having problems after that... you can either ditch your other libraries or recompile them all with this flag
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0")

# For VSCode extention "clangd"
#	CMAKE_EXPORT_COMPILE_COMMANDS generates the "compile_commands.json" file in the build dir
#	Reload the "clangd" extension to make it find the file
#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

####################################################################################
# Host machine dependent
##########################################
# Identify the current computer by checking the hostname
# 	Should be unique in most cases
#	Otherwise, try check "if(WIN32)" or something else unique about the setup
#	Good variables to check:
#		https://cmake.org/cmake/help/latest/command/cmake_host_system_information.html

cmake_host_system_information(RESULT HOSTNAME_THIS_PC QUERY HOSTNAME)
message("BJ: hostname for this comuter: ${HOSTNAME_THIS_PC}")

# !! Change the paths in here to your project location
# !! Then change the string to the hostname of your computer
if("${HOSTNAME_THIS_PC}" STREQUAL "acrv-All-Series")
	# UR5 Computer
	set(BJ_PROJECT_DIR "/path_to_project_root")
	set(BJ_LIB_DIR "${BJ_PROJECT_DIR}/lib_ubuntu")

	# Paths to: Libraries, Includes (library header files), Packages
	#	Vicon Data Stream SDK
	set(VICONDS_LIB_DIR "${BJ_LIB_DIR}" CACHE PATH "dir vicon lib")
	set(VICONDS_INC_DIR "${BJ_LIB_DIR}" CACHE PATH "dir vicon h")

else()
	message(FATAL_ERROR "BJ: Hostname does not match. Setup this host machine in the top-level CMake file")
endif()

# Path for output exe
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BJ_PROJECT_DIR}/bin")

####################################################################################
# Find all libraries & packages
##########################################
# find_library() assumes that for a library "x"
#	Windows filename: x.lib
#	Windows filename: x.dll (shared library)
#	Linux filename: libx.a
#	Linux filename: libx.so (shared library)
# You can change this if you need (google it), but otherwise, input your libraries without the prefix & suffix

# Find libraries (<varName> <fileName> <filePath> DOC <docstring>)
find_library(VICONDS_LIB "ViconDataStreamSDK_CPP" ${VICONDS_LIB_DIR} DOC "libViconDataStreamSDK_CPP lib")

####################################################################################
# Projects to build
##########################################
# Include sub-projects (folders with other CMakeList.txt files)

add_subdirectory ("vicon_template")
#add_subdirectory ("my_other_project_folder")


