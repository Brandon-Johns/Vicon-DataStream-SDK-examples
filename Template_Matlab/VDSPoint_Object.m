classdef VDSPoint_Object < VDSPoint
properties (Access=private)
    % Child markers of this point
    markers_(:,1) VDSPoint_Marker
end
methods
    % Constructor
    function this = VDSPoint_Object(varargin)
        this = this@VDSPoint(varargin{:});
    end

    %********************************************************************************************************
    % Set
    %****************************************************
    function this = AppendMarkers(this, markers)
        this.markers_ = [this.markers_; markers(:)];
    end

    %********************************************************************************************************
    % Get scalar properties
    % OUTPUT: Array of properties (with same dimension as the array of objects)
    %****************************************************
    function out = Marker(this, idxChild)
        arguments
            this
            idxChild(1,1)
        end
        childrenArray=[this.markers_];
        out = this.PropArray(childrenArray(idxChild,:));
    end

    function out = NumMarkersOnObject(this)
        out = zeros(size(this));
        for idx = 1:numel(this)
            out(idx) = length(this(idx).markers_);
        end
    end

    %********************************************************************************************************
    % Get vector/matrix/array properties
    % OUTPUT: Cell array of properties (with same dimension as the array of objects)
    %****************************************************
    function out = Markers(this); out = this.PropArray( {this.markers_} ); end

    %********************************************************************************************************
    % Helpers
    %****************************************************
    
end
methods (Static)
    % Create new object of this class
    %   Use when class type is not known e.g. array(end+1)=this.new()
    function out = new(varargin)
        out = VDSPoint_Object(varargin{:});
    end
end
end