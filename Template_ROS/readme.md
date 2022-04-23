Vicon DataStream SDK - Template Projects (Not Official)
# ROS (Robot Operating System)
It is recommended to not use the Vicon DataStream SDK with ROS

Instead, it is suggested to use the [vrpn_client_ros](http://wiki.ros.org/vrpn_client_ros) package

## Why not to use the DataStream SDK with ROS
The C++ version of Vicon DataStream SDK has issues with compatibility with most other C++ libraries.

Problem overview
- In 2011, the C++11 standard broke backwards compatibility for std::string
- The organisations who write compilers all took this differently, but most adopted it as the default compile option
	- e.g. GCC gives an option to choose to use the new or old version (See [here for details](https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html))
- Using libraries compiled different versions can sometimes cause errors
- Vicon compiled the SDK with the old version
	- Hence, most other libraries compiled after 2011 will break when used simultaneously with the VDS SDK


Due to this issue, the Vicon DataStream SDK is not compatible with ROS

## Guide to `vrpn_client_ros`
Using the [VRPN protocol](https://en.wikipedia.org/wiki/VRPN), objects tracked by Vicon can be streamed live through ROS topics using the `vrpn_client_ros` package. Vicon Tracker is, by default, enabled to stream data over the network with this protocol.

Install
- Terminal:
	- `sudo apt-get install ros-[your ros version]-vrpn-client-ros`

Run
- Terminal:
	- `roslaunch vrpn_client_ros simple.launch server:=[Vicon Control Computer IP address]`
- Data should then begin to be published to the topics `[trackerObjectName]/pose`
