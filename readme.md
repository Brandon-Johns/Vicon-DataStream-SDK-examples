# Vicon DataStream SDK - Template Projects (Not Official)
This repository contains instruction and template projects to use the Vicon DataStream SDK to stream 'object' positions and orientations from the software Vicon Tracker 3 to user written software in realtime.

Each template has it's own instructions, in the readme file
- [Matlab](./Template_Matlab/readme.md)
- [C++](./Template_CPP/readme.md) (Using CMake)
- [ROS](./Template_ROS/readme.md) (Robot Operating System)

Related Software
- [Vicon Tracker](https://www.vicon.com/software/tracker/)
	- Any version should work
- [Vicon DataStream SDK](https://www.vicon.com/software/datastream-sdk/)
	- This project was written for Version `1.11.0`. Past or future versions may not function the same.

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
