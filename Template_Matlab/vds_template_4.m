%{
Written By: Brandon Johns
Date Version Created: 2022-07-25
Date Last Edited: 2022-07-25
Status: functional

%%% PURPOSE %%%
Example for using Brandon's interface to the "Vicon DataStream SDK"

Output object location & orientation to excel
Also, the object's marker positions

Output in realtime
Press CTRL+C to terminate

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

%myObjectNames = ["Jackle", "Pedestrian"];
myObjectNames = ["bj_ctrl", "arm"];

fileNameOut = "tmp4.xlsx";

%********************************************************************************************************
% Main program
%****************************************************
% Program configuration
hostName = 'localhost:801'; % IP address of the computer running Vicon Tracker
lightWeightMode = false;

% Instance the interface
vdsi = VDSInterface(hostName, lightWeightMode).Connect;
[points, frameInfo] = vdsi.GetFrame;
frameRate = frameInfo.frameRate;

% Initialise the output
ExcelSheets = VDSExporter.empty;
for point = points.GetByName(myObjectNames)
    ExcelSheets(end+1) = VDSExporter(fileNameOut, point.Name, frameRate, "WithMarkers");
    ExcelSheets(end).AutoSheetHead(point);
    ExcelSheets(end).Write_SheetHead;
end

% Capture and write data
frameNumberStart = nan;
while true
    % Get next data frame
    [pointsAll, frameInfo] = vdsi.GetFrame;
    points = points.GetByName(myObjectNames);

    % Offset frameNumber to start at 1
    if isnan(frameNumberStart); frameNumberStart=frameInfo.frameNumber; end
    frameNumber = frameInfo.frameNumber - frameNumberStart + 1;

    % Write data
    for idxP = 1:length(points)
        ExcelSheets(idxP).Write_AppendRow(frameNumber, points(idxP));
    end
end


