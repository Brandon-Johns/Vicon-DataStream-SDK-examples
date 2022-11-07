# Vicon DataStream SDK - Template Projects (Not Official)
This repository contains instruction and template projects to use the Vicon DataStream SDK to stream 'object' positions and orientations from the software Vicon Tracker 3 to user written software in realtime.

Each template has it's own instructions, in the readme file
- [Python](./Template_Python/readme.md)
- [Matlab](./Template_Matlab/readme.md)
- [C++](./Template_CPP/readme.md) (Using CMake)
- [ROS](./Template_ROS/readme.md) (Robot Operating System)

Related Software
- [Vicon Tracker](https://www.vicon.com/software/tracker/)
	- Any version should work
- [Vicon DataStream SDK](https://www.vicon.com/software/datastream-sdk/)
	- This project was written for Version `1.11.0`. Past or future versions may not function the same.

## Software Architecture
Each folder contains a stand alone template project. The categories are:

### Minimal
Simple single file projects to demonstrate receiving and printing frame data

To use the minimal projects, follow the same setup instructions as in the template projects. Then follow the Vicon DataStream SDK Developers Guide to understand how to use the client

### Template
Provides a interface class [that wraps the SDK](https://en.wikipedia.org/wiki/Wrapper_function) as well as examples on how to use the wrapper class.

Features:
- Instead of SetStreamMode, my interfaces retrieve and cache frames in a background thread, while providing 3 modes of retrieval
	- <code>GetFrame</code>
		- Instantly returns the latest cached frame. Will return the same frame again if called before a new frame arrives
		- Use when the call must not block
		- e.g. need to receive frames while also sending commands to a robot at a higher frequency
	- <code>GetFrame_GetUnread</code>
		- If the cached frame has not yet been read, returns instantly. Otherwise blocks until a new frame arrives
		- Use when blocking is acceptable, but duplicate frames are not
		- e.g. a simple program to receive and write every frame to excel
	- <code>GetFrame_WaitForNew</code>
		- Blocks until a new frame arrives regardless of caching
		- Use to ensure minimal delay between receiving a frame to using its values
		- e.g. Synchronise frame data with reading other sensors
- Data is encoded in objects that can be managed easier while seamlessly handling calls to retrieve data from occluded objects
- Fixes bugs in the bare SDK e.g.
	- Stream mode 'ClientPullPreFetch' blocks when called twice in the same frame interval
	- Subject filtering only causes frames to be marked as occluded. They are still listed
	- (C++) Occluded frames are incorrectly flagged as not occluded


## Prerequisite Setup
**Vicon Control Computer**
(The computer that is connected to the vicon cameras)
1. Vicon Tracker is installed on this computer
2. The Vicon system is setup, calibrated, and running

**Client computer**
(The computer you wish to work on | You may work directly on the Vicon Control Computer. The steps are the same.)
1. This computer is connected to the same network as the Vicon Control Computer
2. Download the Vicon DataStream SDK onto this computer
	- Install instructions are provided per template project
3. Download this git repository onto this computer
4. Follow the instructions for the specific language you wish to use.


## This Documentation
This documentation is written in markdown. Please view it on GitHub or with a markdown viewer.

## License
This work is distributed under the [BSD-3-Clause License](./LICENSE.txt)
