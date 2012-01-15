// automatically generated by wrap on 2012-Jan-15
#include <wrap/matlab.h>
#include <Point3.h>
using namespace geometry;
void mexFunction(int nargout, mxArray *out[], int nargin, const mxArray *in[])
{
  checkArguments("Point3_StaticFunctionRet",nargout,nargin,1);
  double z = unwrap< double >(in[0]);
  Point3 result = Point3::StaticFunctionRet(z);
  out[0] = wrap_shared_ptr(make_shared< Point3 >(result),"Point3");
}
