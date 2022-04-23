Vicon DataStream SDK - Template Projects (Not Official)
# MATLAB
These instructions assume that you are running `MATLAB 2021b` or later on Windows

## Setup
1. Install the Vicon DataStream SDK
	- The install path should place the `dotNET` files on the path `C:\Program Files\Vicon\DataStream SDK\Win64\dotNET`
	- If this is not the path, then modify the `addpath` command in the template file `VDSInterface.m` to match

2. If the computer you are working on is not the Vicon Control Computer, then
	- In the each `vds_template_#.m` file, change the line `hostName = 'localhost:801'` so that `hostName` is equal to the IP address of the Vicon Control Computer (specified as a string)

## Run
In MATLAB, run any of the files
- `vds_template_#.m`

## About this template
I created a [wrapper object](https://en.wikipedia.org/wiki/Wrapper_function) `VDSInterface.m` to interface with the Vicon DataStream SDK for us. This object returns object pose information as objects of type `VDSPoint.m`

Note that my wrapper uses the StreamMode `ViconDataStreamSDK.DotNET.StreamMode.ServerPush`. See the implications of this in the Vicon DataStream SDK Manual. You may change the mode in `VDSInterface.m`, but you're running in matlab, so little difference.

First, instance this object, then connect to the DataStream with
```MATLAB
vdsi = VDSInterface(hostName);
vdsi.Connect;
```

Then retrieve data frames (object pose information corresponding to the camera frame it was seen in) with
```MATLAB
[points, frameInfo] = vdsi.GetFrame;
```

The returned values provide frame information as
```MATLAB
frameInfo.frameRate   % Vicon capture frame rate [Hz]
frameInfo.frameNumber % Number of captured frames since tracker was started [Count]
frameInfo.latency     % Delay in receiving the frame from Tracker [Seconds]

% Name of object, as defined in Vicon Tracker
points.Name

% Is the object occluded in this frame?
points.IsOccluded

% Object Positions & Rotations
% These are given with respect to the Vicon global coordinate system (as defined during calibration)

% Translation [mm] (x, y, z, position vector, homogeneous position vector)
points.x
points.y
points.z
points.P      % OUTPUT: [x; y; z]
points.Ph     % OUTPUT: [x; y; z; 1]

% Rotation (matrix, quaternions, axis angle, euler)
points.R               % OUTPUT: A rotation matrix
points.quat            % OUTPUT: [w, x, y, z]
points.axang           % OUTPUT: [ex, ey, ez, theta]
points.euler(sequence) % INPUT/OUTPUT: See Matlab documentation for "rotm2eul"

% Homogeneous transformation matrix [position in mm]
points.T
```

Additionally, to sort through the returned points, the helper function retrieves points based on their name as defined in Vicon Tracker
```MATLAB
% Get Multiple Points
%   The order of the output matches the order of the input
%   Occluded points are filled with the value nan
MyPoints = points.GetByName(["Robot", "Human"]);

% Get individual points
RobotPose = points.GetByName("Robot");
HumanPose = points.GetByName("Human");
```

