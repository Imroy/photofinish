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
#ifndef __FILTER_HH__
#define __FILTER_HH__

#include "Destination_items.hh"
#include "Exception.hh"

namespace PhotoFinish {

  class _Filter {
  protected:
    double _radius;

  public:
    _Filter(const D_resize& dr) :
      _radius(dr.has_support() ? dr.support() : 3)
    {}
    virtual ~_Filter()
    {}

    inline double radius(void) const { return _radius; }

    virtual SAMPLE eval(double x) const = 0;
  };

  class Lanczos : public _Filter {
  private:
    double _r_radius;	// Reciprocal of the radius

  public:
    Lanczos(const D_resize& dr) :
      _Filter(dr),
      _r_radius(1.0 / _radius)
    {}
    ~Lanczos()
    {}

    SAMPLE eval(double x) const;
  };

  // Factory function
  _Filter* Filter(const D_resize& resize) throw(DestinationError);

}

#endif /* __FILTER_HH__ */
