// automatically generated by wrap on 2012-Jan-15
#include <wrap/matlab.h>
#include <folder/path/to/Test.h>
using namespace geometry;
void mexFunction(int nargout, mxArray *out[], int nargin, const mxArray *in[])
{
  checkArguments("return_int",nargout,nargin-1,1);
  shared_ptr<Test> self = unwrap_shared_ptr< Test >(in[0],"Test");
  int value = unwrap< int >(in[1]);
  int result = self->return_int(value);
  out[0] = wrap< int >(result);
}
