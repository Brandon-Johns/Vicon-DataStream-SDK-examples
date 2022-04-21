# Mid/Low-level CMake project file
#	Project specific logic: include source and define output
cmake_minimum_required (VERSION 3.8)

####################################################################################
# All dependancies
##########################################
set(LIBRARIES
	${VICONDS_LIB})

set(LIBRARIES_DIR 
	${VICONDS_LIB_DIR})

set(INCLUDES_DIR 
	${VICONDS_INC_DIR})

set(INCLUDES_LOCAL_DIR 
	${CMAKE_CURRENT_LIST_DIR})

####################################################################################
# User Input
##########################################
# cpp files containing "main()"
#	e.g. set(BJ_Sources exe1 exe2 ...)
set(BJ_Sources "vds_template_1" "vds_template_2")

# Other cpp files to link with
set(BJ_Dependencies )


####################################################################################
# Build (Automated - do not edit)
##########################################
foreach(BJ_Source ${BJ_Sources})
	# Choose Output filename
	set(BJ_ExeName "${BJ_Source}")

	# Libraries to link against - Directories
	if(WIN32)
		#Nothing
	else() 
		link_directories(${LIBRARIES_DIR})
	endif()

	# Link source files to output file (<exeName> <source1.c> [source2.c|.h] ...)
	add_executable(${BJ_ExeName} ${BJ_Source}.cpp ${BJ_Dependencies})

	# Directories to include
	target_include_directories(${BJ_ExeName} PRIVATE ${INCLUDES_DIR} ${INCLUDES_LOCAL_DIR})

	# Libraries to link against - Libraries
	target_link_libraries(${BJ_ExeName} PUBLIC ${LIBRARIES})
endforeach()


