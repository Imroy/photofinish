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

#include <memory>
#include "Destination_items.hh"
#include "Exception.hh"

namespace PhotoFinish {

  class Base_Filter {
  protected:
    double _radius;

  public:
    Base_Filter() :
      _radius(3)
    {}
    Base_Filter(const D_resize& dr) :
      _radius(dr.has_support() ? dr.support() : 3)
    {}

    inline double radius(void) const { return _radius; }

    virtual SAMPLE eval(double x) const = 0;
  };

  class Lanczos : public Base_Filter {
  private:
    double _r_radius;	// Reciprocal of the radius

  public:
    Lanczos(const D_resize& dr) :
      Base_Filter(dr),
      _r_radius(1.0 / _radius)
    {}

    SAMPLE eval(double x) const;
  };

  // Factory/wrapper class
  class Filter : public Base_Filter {
  private:
    typedef std::shared_ptr<Base_Filter> ptr;

    ptr _filter;

  public:
    Filter();
    Filter(const D_resize& resize) throw(DestinationError);

    inline SAMPLE eval(double x) const {
      if (_filter == NULL)
	throw Uninitialised("Filter");
      return _filter->eval(x);
    }
  };

}

#endif /* __FILTER_HH__ */
