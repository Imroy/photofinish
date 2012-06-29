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

  Filter::Filter() :
    _filter(NULL)
  {}

  Filter::Filter(const D_resize& dr) throw(DestinationError) {
    if (!dr.has_filter()) {
      _filter = new Lanczos(dr);
      return;
    }

    std::string filter = dr.filter();
    if (boost::iequals(filter.substr(0, min(filter.length(), 7)), "lanczos")) {
      _filter = new Lanczos(dr);
      return;
    }

    throw DestinationError("resize.filter", filter);
  }

  Filter::Filter(const Filter& other) :
    _filter(other._filter)
  {}

  Filter::~Filter() {
    if (_filter != NULL) {
      delete _filter;
      _filter = NULL;
    }
  }
}
