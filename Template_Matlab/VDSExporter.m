classdef VDSExporter < handle
properties (Access=public)
    fileName(1,1) string
    sheetName(1,1) string
    sheetHead
    sheetData
end
properties (Access=private)
    withMarkers(1,1) logical
    frameRate(1,1) double
end
methods
    % Constructor
    function this = VDSExporter(fileName, sheetName, frameRate, mode)
        arguments
            fileName
            sheetName
            frameRate
            mode(1,1) string {mustBeMember(mode, ["NoMarkers","WithMarkers"])} = "noMarkers"
        end
        this.fileName = fileName;
        this.sheetName = sheetName;
        this.frameRate = frameRate;
        this.withMarkers = mode=="WithMarkers";
    end

    %********************************************************************************************************
    % Set
    %****************************************************
    function AutoSheetHead(this, point)
        arguments
            this(1,1)
            point(1,1) VDSPoint_Object
        end
        this.sheetHead = ["Frame","Time (s)", "x (mm)","y (mm)","z (mm)","Rw","Rx","Ry","Rz"];
        if this.withMarkers
            for markerIdx = 1:point.NumMarkersOnObject
                this.sheetHead = [this.sheetHead, ["x_","y_","z_"] + point.Marker(markerIdx).Name];
            end
        end
    end

    function newData = AutoAppendRows(this, frameNumber, points)
        arguments
            this(1,1)
            frameNumber(:,1) double % double to prevent entire row from converting to uint32
            points(:,1) VDSPoint_Object
        end
        frameTime = (frameNumber-1)/this.frameRate;
        newData = [frameNumber, frameTime, points.x, points.y, points.z, cell2mat(points.quat)];
        if this.withMarkers
            for idx = 1:points(1).NumMarkersOnObject
                newData = [newData, points.Marker(idx).x, points.Marker(idx).y, points.Marker(idx).z];
            end
        end
        this.sheetData = [this.sheetData; newData];
    end


    %********************************************************************************************************
    % Output - All at once
    %****************************************************
    function Write_OverwriteSheet(this)
        arguments
            this(1,1)
        end
        dataOut = array2table(this.sheetData, 'VariableNames',this.sheetHead);
        writetable(...
            dataOut,...
            this.fileName,...
            'Sheet', this.sheetName,...
            'WriteMode',"overwritesheet",...
            'useExcel', false...
            );
    end

    %********************************************************************************************************
    % Output - Incrementally
    %****************************************************
    function Write_SheetHead(this)
        writematrix(...
            this.sheetHead,...
            this.fileName,...
            'Sheet', this.sheetName,...
            'WriteMode',"overwritesheet",...
            'useExcel', false...
            );
    end

    function Write_AppendRow(this, varargin)
        writematrix(...
            this.AutoAppendRows(varargin{:}),...
            this.fileName,...
            'Sheet', this.sheetName,...
            'WriteMode',"append",...
            'useExcel', false...
            );
    end
end
end