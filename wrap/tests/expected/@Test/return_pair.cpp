// automatically generated by wrap on 2012-Jan-15
#include <wrap/matlab.h>
#include <folder/path/to/Test.h>
using namespace geometry;
void mexFunction(int nargout, mxArray *out[], int nargin, const mxArray *in[])
{
  checkArguments("return_pair",nargout,nargin-1,2);
  shared_ptr<Test> self = unwrap_shared_ptr< Test >(in[0],"Test");
  Vector v = unwrap< Vector >(in[1]);
  Matrix A = unwrap< Matrix >(in[2]);
  pair< Vector, Matrix > result = self->return_pair(v,A);
  out[0] = wrap< Vector >(result.first);
  out[1] = wrap< Matrix >(result.second);
}
