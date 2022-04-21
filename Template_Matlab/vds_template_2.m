%{
Written By: Brandon Johns
Date Version Created: 2022-04-21
Date Last Edited: 2022-04-21
Status: functional
Dependencies:
    VDSInterface.m
    VDSPoint.m

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

% Get next data frame
[points, frameInfo] = vdsi.GetFrame;

frameRate = frameInfo.frameRate %     [Hz]
frameNumber = frameInfo.frameNumber % [Count]
latency = frameInfo.latency %         [Seconds]

% Get info about an object
myObjects = points.GetByName(["Jackle", "Pedestrian"]);

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




