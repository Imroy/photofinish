/*
	Copyright 2012 Ian Tester

	This file is part of Photo Finish.

	Photo Finish is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Photo Finish is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Photo Finish.  If not, see <http://www.gnu.org/licenses/>.
*/
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
