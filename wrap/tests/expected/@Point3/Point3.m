classdef Point3
  properties
    self = 0
  end
  methods
    function obj = Point3(varargin)
      if (nargin == 3 && isa(varargin{1},'double') && isa(varargin{2},'double') && isa(varargin{3},'double')), obj.self = new_Point3_ddd(varargin{1},varargin{2},varargin{3}); end
      if nargin ~= 13 && obj.self == 0, error('Point3 constructor failed'); end
    end
    function display(obj), obj.print(''); end
    function disp(obj), obj.display; end
  end
end
