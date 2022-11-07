%{
Written By: Brandon Johns
Date Version Created: 2022-11-07
Date Last Edited: 2022-11-07
Status: NOT TESTED

%%% PURPOSE %%%
Template for using Vicon DataStream (VDS)
Minimal example based on the example that comes with the SDK

%%% TODO %%%

%%% NOTES %%%

%}
close all;
clear all;
clc
%********************************************************************************************************
% Config
%****************************************************
% Program configuration
hostName = 'localhost:801'; % IP address of the computer running Vicon Tracker


%********************************************************************************************************
% Initialise
%****************************************************
% Load the SDK
addpath( 'C:\Program Files\Vicon\DataStream SDK\Win64\dotNET' );
path_VDS_DLL = which('ViconDataStreamSDK_DotNET.dll');
if ~exist(path_VDS_DLL, 'file'); error("VDS_ERROR: SDK DLL not found"); end
NET.addAssembly(path_VDS_DLL);

% Connect to server
client = ViconDataStreamSDK.DotNET.Client;
client.Connect(hostName);
if ~client.IsConnected().Connected; error("VDS_ERROR: Failed to connect to Vicon Tracker\n"); end

% Enable client options
client.EnableSegmentData();
client.SetBufferSize(1); % Always return most recent frame
client.SetStreamMode(ViconDataStreamSDK.DotNET.StreamMode.ServerPush);
client.SetAxisMapping( ...
    ViconDataStreamSDK.DotNET.Direction.Forward, ...
    ViconDataStreamSDK.DotNET.Direction.Left, ...
    ViconDataStreamSDK.DotNET.Direction.Up); % Set the global up axis Z-UP


%********************************************************************************************************
% Main
%****************************************************
% Get 3 frames
for idx = 1 : 3
    % Retrieve a frame from Vicon Tracker
    client.GetFrame;

    % Decode frame - miscellaneous
    fprintf("Frame Number: " + client.GetFrameNumber.FrameNumber + "\n")
    fprintf("Frame Rate:   " + client.GetFrameRate.FrameRateHz + "\n")
    fprintf("Latency:      " + client.GetLatencyTotal.Total + "\n")

    % Decode frame - object pose
    for subjectIndex_int32 = 0 : int32(client.GetSubjectCount.SubjectCount) - 1
        subjectIndex = uint32(subjectIndex_int32);
    
        subjectName = client.GetSubjectName(subjectIndex).subjectName;
        segmentName = client.GetSubjectRootSegmentName(subjectName).segmentName;

        ret_P = client.GetSegmentGlobalTranslation(subjectName, segmentName);
        ret_R = client.GetSegmentGlobalRotationMatrix(subjectName, segmentName);

        P = double(ret_P.Translation).';
        R_rowMajor = double(ret_R.Rotation);
        R = [R_rowMajor(1:3); R_rowMajor(4:6); R_rowMajor(7:9)];
        isOccluded = ret_P.Occluded || ret_R.Occluded;

        fprintf(subjectName + "\n")
        fprintf("Is Occluded: " + isOccluded + "\n")
        fprintf("Translation:\n")
        disp(P)
        fprintf("Rotation Matrix:\n")
        disp(R)
    end
end


%********************************************************************************************************
% End
%****************************************************
client.Disconnect;
