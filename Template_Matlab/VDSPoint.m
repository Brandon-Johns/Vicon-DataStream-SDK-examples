classdef VDSPoint
properties (Access=private)
    % The Name of the object in Vicon Tracker
    % Flag for if the object was not detected for this frame
    name_(1,1) string
    isOccluded_(1,1) logical

    % Rotation matrix in vicon global frame
    % Position in vicon global frame
    R_(3,3)
    x_(1,1) double
    y_(1,1) double
    z_(1,1) double
end
methods
    % Constructor
    function this = VDSPoint(Name, R, P)
        arguments
            Name
            R = nan(3,3)
            P = nan(3,1)
        end
        this.name_ = Name;
        this.isOccluded_ = all(isnan(R),'all') && all(isnan(P),'all');
        this.R_ = R;
        this.x_ = P(1);
        this.y_ = P(2);
        this.z_ = P(3);
    end

    %********************************************************************************************************
    % Get scalar properties
    % OUTPUT: Array of properties (with same dimension as the array of objects)
    %****************************************************
    function out = Name(this);       out=string(  this.PropArray(this.name_) );       end
    function out = IsOccluded(this); out=logical( this.PropArray(this.isOccluded_) ); end
    function out = x(this);          out=this.PropArray(this.x_); end
    function out = y(this);          out=this.PropArray(this.y_); end
    function out = z(this);          out=this.PropArray(this.z_); end

    %********************************************************************************************************
    % Get vector/matrix/array properties
    % OUTPUT: Cell array of properties (with same dimension as the array of objects)
    %****************************************************
    % Position vector
    % Form: [x; y; z]
    function out = P(this)
        out = cell(size(this));
        for idx = 1:numel(this)
            out{idx} = [this(idx).x_; this(idx).y_; this(idx).z_];
        end
    end

    % Position vector (homogeneous form)
    % Form: [x; y; z; 1]
    function out = Ph(this)
        out = cell(size(this));
        for idx = 1:numel(this)
            out{idx} = [this(idx).x_; this(idx).y_; this(idx).z_; 1];
        end
    end

    % Rotation (matrix)
    function out = R(this); out = this.PropArray( {this.R_} ); end

    % Rotation (quaternions)
    % Form: [w, x, y, z]
    function out = quat(this)
        out = cell(size(this));
        for idx = 1:numel(this)
            out{idx} = rotm2quat( this(idx).R{1} );
        end
    end

    % Rotation (axis angle)
    % Form: [ex, ey, ez, theta]
    function out = axang(this)
        out = cell(size(this));
        for idx = 1:numel(this)
            out{idx} = rotm2axang( this(idx).R{1} );
        end
    end

    % Rotation (euler)
    % Form: See matlab doc for rotm2eul
    function out = euler(this, sequence)
        arguments
            this
            sequence(1,1) string = "XYZ"
        end
        out = cell(size(this));
        for idx = 1:numel(this)
            out{idx} = rotm2eul( this(idx).R{1}, sequence );
        end
    end

    % Transformation matrix
    function out = T(this)
        out = cell(size(this));
        for idx = 1:numel(this)
            out{idx} = [ [this(idx).R_, this(idx).P{1}]; [0,0,0,0] ];
        end
    end

    %********************************************************************************************************
    % Helpers
    %****************************************************
    % Return only the points, from and array of points, that are not occluded
    function pointObject = GetIfNotOccluded(this)
        pointObject = this( ~this.IsOccluded );
    end

    % Get a point in array of points, by it's Name
    % INPUT:
    %   pointsIn
    %       = (string) Array of the values of pointObject.Name
    %       = (VDSPoint) Array of pointObject
    % OUTPUT:
    %   Array of each pointObject, ordered by pointsIn
    function pointObject = GetByName(this, pointsIn, keepNotFound)
        arguments
            this
            pointsIn(1,:) {mustBeA(pointsIn, ["string", "VDSPoint"])}
            keepNotFound(1,1) {mustBeMember(keepNotFound, ["keepNotFound", "discardNotFound"])} = "keepNotFound"
        end
        % Interpret input type (effectively implements input overloading)
        if isa(pointsIn, "VDSPoint")
            nameIn=pointsIn.Name;
        else
            nameIn=pointsIn;
        end

        % Find index of points
        pointIdx = this.GetIdxByName(nameIn);
        notFoundIdx = isnan(pointIdx);

        % Filter not found points per options
        pointObject = VDSPoint.empty;
        if keepNotFound=="keepNotFound"
            for idx = 1 : length(pointIdx)
                if notFoundIdx(idx)
                    % Create not found points as occluded
                    pointObject = [ pointObject, VDSPoint( nameIn(idx) ) ];
                else
                    pointObject = [ pointObject, this( pointIdx(idx) ) ];
                end
            end
        else % keepNotFound==discardNotFound
            if any(notFoundIdx); warning("Some points not found"); end
            pointIdx(notFoundIdx) = [];
            pointObject = this(pointIdx);
        end
    end

end
methods (Access=private)
    % Get positions of points in array of point objects, by point name
    function idxMatches = GetIdxByName(this, nameIn)
        arguments
            this
            nameIn
        end

        % This method will match the order in the input, removing not found, but keeping duplicate points
        idxMatches = nan(size(nameIn));
        for idxIn = 1:length(nameIn)
            idxMatch = find( nameIn(idxIn)==this.Name );
            if ~isempty(idxMatch)
                idxMatches(idxIn) = idxMatch;
            end
        end
    end

    % Helper to allow using functions with an array of objects
    % Only intended for use on scalar properties. Place vector properties in a cell array
    % Input: List of properties
    % OUTPUT: Array of properties corresponding to the object
    % EXAMPLE: PropArray(this.prop)
    function propArray = PropArray(this, varargin)
        propArray = reshape([varargin{:}],size(this));
    end
end
end