%{
Written By: Brandon Johns
Date Version Created: 2022-04-14
Date Last Edited: 2022-04-29
Status: functional

%%% PURPOSE %%%
Example for using Brandon's interface to the "Vicon DataStream SDK"

Print locations of objects to the screen

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
vdsi = VDSInterface(hostName, lightWeightMode).Connect;

% Collect data
while true
    % Get next data frame
    [points, frameInfo] = vdsi.GetFrame;

    %********************************************************************************************************
    % Do something with the output data
    %   Brandon's interface makes it easy to work with the data!
    %****************************************************
    % Operate on a subset of the points
    jackal = points.GetByName("Jackal");
    pedestrian = points.GetByName("Pedestrian");
    fprintf("%d\t", frameInfo.frameNumber);
    fprintf("%s:[%8.1f, %8.1f, %8.1f]\t", jackal.Name, jackal.x, jackal.y, jackal.z);
    fprintf("%s:[%8.1f, %8.1f, %8.1f]\t", pedestrian.Name, pedestrian.x, pedestrian.y, pedestrian.z);
    fprintf("\n");
end

