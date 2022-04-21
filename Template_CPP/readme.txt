Vicon DataStream SDK (C++)
	Template
	Examples

Written by: Brandon

Using Vicon DataStream SDK version: 1.11.0

Note
	I wrote this to be friendly if it's your first CMake project
	The C++ may be a little difficult to beginners. Focus on the usage in the files with main()
	umm... have fun?

####################################################################################
Getting started
##########################################
Copy this folder "Template_CPP" onto your computer. It will be your project root folder

Adding the SDK
	Download the Vicon DataStream SDK
		https://www.vicon.com/software/datastream-sdk/?section=downloads
	From the Linux64 Dir, extract "ViconDataStreamSDK_1.11.0.128037h__Linux64.zip"
	Place the extracted files into "Template_CPP\lib_ubuntu"

Configure bashrc file to run the helper scripts
	Follow the setup and activate instructions in "Template_CPP\scripts\bashrc_append.sh"

Configure the root CMake file
		"Template_CPP\src\CMakeLists.txt"
	Change the lines
		if("${HOSTNAME_THIS_PC}" STREQUAL "acrv-All-Series")
		set(BJ_PROJECT_DIR "/path_to_project_root")
	To
		replace "acrv-All-Series" with the hostname of the computer you're installing this on
		replace "/path_to_project_root" with the absolute path to this folder "Template_CPP"

Configure the example files
		"Template_CPP\src\vicon_template\vds_template_1.cpp"
		"Template_CPP\src\vicon_template\vds_template_2.cpp"
	Change the lines
		constexpr auto NAME_MyViconObject1 = "bj_c1_bh";
		constexpr auto vds_HostName = "192.168.11.3";
	To
		The name of the object defined in Vicon Tracker
		The IP address of the computer running Vicon Tracker

Build and run
	In terminal, run
		bjbuild
	Navigate to the "Template_CPP\bin", and run the 2 files
		./vds_template_1
		./vds_template_2


####################################################################################
Getting Started with Modifications
##########################################
Add more project folders to the root CMake file
Add more source files to the other CMake file
Read "VDS_Interface.h" to see what else it can do and add more functionality as you require - I added lots of comments
Use a maths library that can work with matrices (I use the Armadillo C++ Library)


####################################################################################
Troubleshooting
##########################################
The C++ version of Vicon DataStream SDK has issues with compatibility with most other C++ libraries.

Problem overview
	In 2011, the C++11 standard broke backwards compatibility for std::string
	The organisations who write compilers all took this differently, but most adopted it as the default compile option
		e.g. GCC gives an option to choose to use the new or old version
		https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html
	Using libraries compiled different versions can sometimes cause errors
	Vicon compiled the SDK with the old version
		Hence, most other libraries compiled after 2011 will break when used simultaneously with the VDS SDK

Problem diagnosis (GCC)
	I added to the CMake file:
		# Enable backwards compatibility Warnings
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wabi-tag")
	This warns you if you are using mixed old and new versions
		If you're not getting any random segmentation faults, then you can ignore the Warnings
		but otherwise, now you know the problem
	I also added
		# Enable backwards compatibility mode
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0")
	Switching this from 0 to 1 toggles the compatibility mode

Solution
	Either 1) try to compile the SDK with the new version or 2) compile all your other libraries with the old version, so that they match
		1) I had a go and it gave errors. I didn't try to fix, but maybe worth looking into? Maybe wait until Vicon fix it?
		2) I managed to do this for the libraries ur_rtde and Boost. Works great, but was painful to do
	If you're using ROS, just use this instead: http://wiki.ros.org/vrpn_client_ros



