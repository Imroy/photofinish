#include <math.h>
#include <boost/algorithm/string.hpp>
#include "Filter.hh"

#define sqr(x) ((x) * (x))
#define min(x,y) ((x) < (y) ? (x) : (y))

namespace PhotoFinish {

  SAMPLE Lanczos::eval(double x) const {
    if (fabs(x) < 1e-6)
      return 1.0;
    double pix = M_PI * x;
    return (_radius * sin(pix) * sin(pix * _r_radius)) / (sqr(M_PI) * sqr(x));
  }

  _Filter* Filter(const D_resize& dr) throw(DestinationError) {
    std::string filter = dr.filter();
    if (boost::iequals(filter.substr(0, min(filter.length(), 7)), "lanczos"))
      return new Lanczos(dr);

    throw DestinationError("resize.filter", filter);
  }

}
