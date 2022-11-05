classdef VDSPoint_Marker < VDSPoint
methods
    % Constructor
    function this = VDSPoint_Marker(Name, P)
        arguments
            Name
            P = nan(3,1)
        end
        this = this@VDSPoint(Name, nan(3,3), P);
    end

    %********************************************************************************************************
    % Helpers
    %****************************************************
    
end
methods (Static)
    % Create new object of this class
    %   Use when class type is not known e.g. array(end+1)=this.new()
    function out = new(varargin)
        out = VDSPoint_Marker(varargin{:});
    end
end
end