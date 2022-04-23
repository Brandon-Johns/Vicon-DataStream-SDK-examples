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

Output object location & orientation to excel

%%% TODO %%%

%%% NOTES %%%

%}
close all;
clear all;
clc

%********************************************************************************************************
% User options
%****************************************************
runDuration = 2; % [seconds]

myObjectNames = ["Jackle", "Pedestrian"];

fileNameOut = "tmp.xlsx";

%********************************************************************************************************
% Main program
%****************************************************
% Program configuration
hostName = 'localhost:801'; % IP address of the computer running Vicon Tracker
lightWeightMode = false;

% Instance the interface
vdsi = VDSInterface(hostName, lightWeightMode).Connect;
[~, frameInfo] = vdsi.GetFrame;
frameRate = frameInfo.frameRate;

frameNumber = zeros(runDuration*frameRate, 1);
points = VDSPoint.empty;
for idx = 1 : length(frameNumber)
    % Get next data frame
    [pointsAll, frameInfo] = vdsi.GetFrame;
    frameNumber(idx) = frameInfo.frameNumber;
    points = [points; pointsAll.GetByName(myObjectNames)];
end

% Offset frameNumber to start at 1
% Find corresponding frame times
frameNumber = frameNumber - frameNumber(1) + 1;
frameTime = (frameNumber-1)/frameRate;

% Write all captured data
for idxP = 1:size(points,2)
    % Data to output
    valuesOut = [frameNumber, frameTime, points(:,idxP).x, points(:,idxP).y, points(:,idxP).z, cell2mat(points(:,idxP).quat)];
    namesOut = ["Frame","Time (s)", "x (mm)","y (mm)","z (mm)","Rw","Rx","Ry","Rz"];
    % Write output
    dataOut = array2table(valuesOut, 'VariableNames',namesOut);
    writetable(...
        dataOut,...
        fileNameOut,...
        'Sheet', points(1,idxP).Name,...
        'WriteMode',"inplace",...
        'useExcel', false...
        );
end


