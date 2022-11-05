%{
Written By: Brandon Johns
Date Version Created: 2022-07-25
Date Last Edited: 2022-07-25
Status: functional

%%% PURPOSE %%%
Example for using Brandon's interface to the "Vicon DataStream SDK"

Output object location & orientation to excel
Also, the object's marker positions

%%% TODO %%%

%%% NOTES %%%

%}
close all;
clear all;
clc

%********************************************************************************************************
% User options
%****************************************************
runDuration = 1; % [seconds]

%myObjectNames = ["Jackal", "Pedestrian"];
myObjectNames = ["bj_ctrl", "arm"];

fileNameOut = "tmp3.xlsx";

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

% Capture data
frameNumber = zeros(runDuration*frameRate, 1);
points = VDSPoint_Object.empty;
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
    % Object data
    valuesOut = [frameNumber, frameTime, points(:,idxP).x, points(:,idxP).y, points(:,idxP).z, cell2mat(points(:,idxP).quat)];
    namesOut = ["Frame","Time (s)", "x (mm)","y (mm)","z (mm)","Rw","Rx","Ry","Rz"];

    % Marker data
    for markerIdx = 1:points(1,idxP).NumMarkersOnObject
        marker = points(:,idxP).Marker(markerIdx);
        valuesOut = [valuesOut, marker.x, marker.y, marker.z];
        namesOut = [namesOut, ["x_","y_","z_"]+marker(1).Name];
    end

    % Write output
    dataOut = array2table(valuesOut, 'VariableNames',namesOut);
    writetable(...
        dataOut,...
        fileNameOut,...
        'Sheet', points(1,idxP).Name,...
        'WriteMode',"overwritesheet",...
        'useExcel', false...
        );
end


