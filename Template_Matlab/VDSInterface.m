classdef VDSInterface < handle
properties
    vdsClient(1,1) % No property validation because the DLL hasn't been added to the path yet

    %********************************************************************************************************
    % User options
    %****************************************************
    % If enabled, GetFrame() will not return occluded subjects
    Filter_DiscardOccludedSubjects(1,1) logical = false;

    % GetFrame() will only return objects in this list
    %   If empty, GetFrame() will return all subjects
    Filter_PermittedSubjects(:,1) string = []
end
properties (Access=private)
    hostName(1,1) string
    lightWeightMode(1,1) logical
end
methods
    % Constructor
    function this = VDSInterface(hostName, lightWeightMode)
        arguments
            hostName(1,1) string = "localhost:801"
            lightWeightMode(1,1) logical = false
        end
        this.hostName = hostName;
        this.lightWeightMode = lightWeightMode;

        % Load the SDK
        addpath( 'C:\Program Files\Vicon\DataStream SDK\Win64\dotNET' );
        path_VDS_DLL = which('ViconDataStreamSDK_DotNET.dll');
        if ~exist(path_VDS_DLL, 'file'); error("SDK DLL not found"); end
        NET.addAssembly(path_VDS_DLL);

        this.Connect;
    end

    function this = Connect(this)
        % Connect to server
        this.vdsClient = ViconDataStreamSDK.DotNET.Client;
        this.vdsClient.Connect( this.hostName );
        if ~this.vdsClient.IsConnected().Connected; error("BJ_ERROR: (VDS) Failed to connect to Vicon Tracker\n"); end

        % Enable client options
        this.vdsClient.EnableSegmentData();

        if this.lightWeightMode
            ret = this.vdsClient.EnableLightweightSegmentData;
            if ret.Result ~= ViconDataStreamSDK.DotNET.Result.Success
                warning("BJ_WARN: (VDS) Failed to enable lightweight segment data.\n");
            end
        end

        % Always return most recent frame
        this.vdsClient.SetBufferSize(1);

        % Stream mode
        %this.vdsClient.SetStreamMode( ViconDataStreamSDK.DotNET.StreamMode.ClientPull );
        %this.vdsClient.SetStreamMode( ViconDataStreamSDK.DotNET.StreamMode.ClientPullPreFetch );
        this.vdsClient.SetStreamMode( ViconDataStreamSDK.DotNET.StreamMode.ServerPush );

        % Set the global up axis Z-UP
        this.vdsClient.SetAxisMapping( ...
            ViconDataStreamSDK.DotNET.Direction.Forward, ...
            ViconDataStreamSDK.DotNET.Direction.Left, ...
            ViconDataStreamSDK.DotNET.Direction.Up );
    end

    function this = Disconnect(this)
        this.vdsClient.Disconnect;
    end

    function [points, frameInfo] = GetFrame(this)
        % Return array
        points = VDSPoint.empty;
        frameInfo = struct('frameRate',nan, 'frameNumber',nan, 'latency',nan);

        % Get next frame from DataStream
        ret = this.vdsClient.GetFrame;
        if ret.Result ~= ViconDataStreamSDK.DotNET.Result.Success
            warning("BJ_ERROR: (VDS) Lost connection to Vicon Tracker");
            return; % Return an empty data frame & frameInfo values are nan
        end

        % Misc frame information
        frameInfo.frameRate = this.vdsClient.GetFrameRate.FrameRateHz;
        frameInfo.frameNumber = this.vdsClient.GetFrameNumber.FrameNumber;
        frameInfo.latency = this.vdsClient.GetLatencyTotal.Total;

        % Loop over all subjects
        %   Casting uint32->int32 for special case of empty array
        SubjectCount = this.vdsClient.GetSubjectCount.SubjectCount;
        for SubjectIndex_int32 = 0 : int32(SubjectCount) - 1
            SubjectIndex = uint32(SubjectIndex_int32);

            SubjectName = this.vdsClient.GetSubjectName( SubjectIndex ).SubjectName;
            SegmentName = this.vdsClient.GetSubjectRootSegmentName( SubjectName ).SegmentName;
            
            % Number of segments should always be 1 when using Vicon Tracker3... as far as I can tell
            SegmentCount = this.vdsClient.GetSegmentCount( SubjectName ).SegmentCount;
            if SegmentCount~=1
                % If this happens, then you get to rewrite this interface to also loop over and store Segment data
                % Refer to the example that comes with the Vicon DataStream SDK
                error("BJ_ERROR: (VDS) Invalid assumption that segment count is 1");
            end

            % Global translation
            retP = this.vdsClient.GetSegmentGlobalTranslation( SubjectName, SegmentName );
            P = double(retP.Translation);
            
            % Global rotation matrix
            %   Note: Vicon uses row major order
            retR = this.vdsClient.GetSegmentGlobalRotationMatrix( SubjectName, SegmentName );
            R_rowMajor = double(retR.Rotation);
            R = [ R_rowMajor(1:3); R_rowMajor(4:6); R_rowMajor(7:9) ];

            % Test if object is occluded
            isOccluded = retP.Occluded || retR.Occluded;
            
            % Append to list of points
            % Ignore occluded points. They'll be added back on calls to VDSPoint.GetByName()
            if ~isOccluded
                points = [ points, VDSPoint(SubjectName, isOccluded, R, P) ];
            end
        end

        % Apply filters
        if ~isempty(this.Filter_PermittedSubjects)
            points = points.GetByName(this.Filter_PermittedSubjects);
        end
        if this.Filter_DiscardOccludedSubjects
            points = points.GetIfNotOccluded;
        end
    end
end
end