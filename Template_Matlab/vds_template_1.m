%{
Written By: Brandon Johns
Date Version Created: 2022-04-21
Date Last Edited: 2022-07-25
Status: functional

%%% PURPOSE %%%
Example for using Brandon's interface to the "Vicon DataStream SDK"

Demonstrate use of all functions

%%% TODO %%%

%%% NOTES %%%

%}
close all;
clear all;
clc

% Program configuration
hostName = 'localhost:801'; % IP address of the computer running Vicon Tracker
lightWeightMode = false;

% Instance the interface
vdsi = VDSInterface(hostName, lightWeightMode);
vdsi.Connect;

% Get next data frame
[points, frameInfo] = vdsi.GetFrame;

frameRate = frameInfo.frameRate %     [Hz]
frameNumber = frameInfo.frameNumber % [Count]
latency = frameInfo.latency %         [Seconds]

% Get info about specific objects (Use the names of the objects as specified in Vicon Tracker)
%myObjects = points.GetByName(["Jackal", "Pedestrian"]);
myObjects = points

% Optionally filter out occluded points
%myObjects = myObjects.GetIfNotOccluded;

% Name of object, as defined in Vicon Tracker
name = myObjects.Name

% Is the object occluded in this frame?
IsOccluded = myObjects.IsOccluded

% Translation [mm] (x, y, z, position vector, homogeneous position vector)
x = myObjects.x
y = myObjects.y
z = myObjects.z
P = myObjects.P
Ph = myObjects.Ph

% Rotation (matrix, quaternions, axis angle, euler)
R = myObjects.R{:}
quat = myObjects.quat
axang = myObjects.axang
euler = myObjects.euler

% Transformation matrix [position in mm]
T = myObjects.T{:}

% List Markers
for object = myObjects
    Markers = table(object.Markers.P{:}, 'variableNames', object.Markers.Name.', 'rowNames',["x","y","z"])
end

return

figure
hold on
plotTransforms([0,0,0], [1,0,0,0])
plotTransforms(points.GetByName("Jackal").P{1}.'/1000, points.GetByName("Jackal").quat{1})
xlabel("x (m)")
xlabel("y (m)")
xlabel("z (m)")
grid on
hold off
