Vicon DataStream SDK - Template Projects (Not Official)
# C++

I wrote this to be friendly if it's your first CMake project

## Setup (Windows)
1. Install the Vicon DataStream SDK
	- Download the Vicon DataStream SDK
	- Run the Win64 installer
	- The C++ DLLs should install to `C:/Program Files/Vicon/DataStream SDK/Win64/CPP`

2. Configure the Windows PATH
	- Option 1 (Recommended): Add `C:/Program Files/Vicon/DataStream SDK/Win64/CPP` to the Windows PATH Variable
	- Option 2: Copy paste the C++ DLLs to `Template_CPP/bin`
	- Option 3: Whenever executing a built exe, use the powershell script `Template_CPP\scripts\VDSI_runExe.ps1`

3. Configure the example files `Template_CPP/src/vicon_template/vds_template_#.cpp`
	- Change the lines
		- `constexpr auto NAME_MyViconObject1 = "bj_c1_bh";`
		- `constexpr auto vds_HostName = "192.168.11.3";`
	- To
		- The name of the object defined in Vicon Tracker
		- The IP address of the Vicon Control Computer

## Build and run (Windows)
1. Open Visual Studio
2. Select `File > Open > CMake...`, then select `Template_CPP\src\CMakeLists.txt`
3. Select `Build > Build All`
4. Open a file `vds_template_#`
3. Select `Debug > Start Debugging` or `Debug > Start Without Debugging`

or use the ps1 scripts (but these don't offer any debugging tools)

## Setup (Linux)
NOTE: This folder `Template_CPP` will be your project root folder

1. Install the Vicon DataStream SDK
	- Download the Vicon DataStream SDK
	- From the Linux64 Dir, extract `ViconDataStreamSDK_1.11.0.128037h__Linux64.zip`
	- Place the extracted files into `Template_CPP/lib_ubuntu/vds`

2. Configure `~./bashrc` to run the helper scripts
	- Follow the setup and activate instructions in `Template_CPP/scripts/VDSI_bashrc_append.sh`

3. Configure the root CMake file `Template_CPP/src/CMakeLists.txt`
	- If necessary, change the variables to reflect your install paths
		- `VICONDS_LIB_DIR` is the path to the `.so` files in the CPP directory
		- `VICONDS_INC_DIR` is the path to the `.h` files in the CPP directory

4. Configure the example files `Template_CPP/src/vicon_template/vds_template_#.cpp`
	- Change the lines
		- `constexpr auto NAME_MyViconObject1 = "bj_c1_bh";`
		- `constexpr auto vds_HostName = "192.168.11.3";`
	- To
		- The name of the object defined in Vicon Tracker
		- The IP address of the Vicon Control Computer

## Build and run (Linux)
1. In terminal, run
	- `vdsibuild`
2. Navigate to the `Template_CPP/bin`, and run the 2 files
	- `./vds_template_1`
	- `./vds_template_2`


## Getting Started with Modifications
Read the descriptions of the interfaces in `VDS_Interface.h` to see what else it can do and add more functionality as you require

Alter the class `vdsi::Point` to use a maths library that can work with matrices (I use the Armadillo C++ Library)

## Troubleshooting
The C++ version of Vicon DataStream SDK has issues with compatibility with most other C++ libraries.

Problem overview
- In 2011, the C++11 standard broke backwards compatibility for std::string
- The organisations who write compilers all took this differently, but most adopted it as the default compile option
	- e.g. GCC gives an option to choose to use the new or old version (See [here for details](https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html))
- Using libraries compiled different versions can sometimes cause errors
- Vicon compiled the SDK with the old version
	- Hence, most other libraries compiled after 2011 will break when used simultaneously with the VDS SDK

Problem diagnosis (GCC)
- I added to the CMake file:
	- `# Enable backwards compatibility Warnings`
	- `set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wabi-tag")`
- This warns you if you are using mixed old and new versions
	- If you're not getting any random segmentation faults, then you can ignore the warnings
	- but otherwise, now you know the problem
- I also added
	- `# Enable backwards compatibility mode`
	- `set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0")`
- Switching this from 0 to 1 toggles the compatibility mode

Solution
- Either
	1. Try to compile the SDK with the new version
		- I had a go and it gave errors. I didn't try to fix, but maybe worth looking into? Maybe wait until Vicon fix it?
	2. Compile all your other libraries with the old version, so that they match
		- I managed to do this for the libraries ur_rtde and Boost. Works great, but was painful to do
	3. If you're using ROS, see the Template_ROS readme



