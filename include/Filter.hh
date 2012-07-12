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
#include "sample.h"

namespace PhotoFinish {

  //! Abstract base class for filters
  class Filter {
  protected:

  public:
    typedef std::shared_ptr<Filter> ptr;

    //! Empty constructor
    Filter() {}

    //! Named constructor
    /*! Create a Filter object using the filter name in the D_resize object.
      \param dr A D_resize object which will supply our parameters.
    */
    static ptr create(const D_resize& dr) throw(DestinationError);

    //! The size of this filter
    virtual double range(void) const = 0;

    //! Evaluate the filter at a given point
    virtual SAMPLE eval(double x) const throw(Uninitialised) = 0;
  };

  //! Lanczos filter
  class Lanczos : public Filter {
  private:
    bool _has_radius;
    double _radius, _r_radius;	// Radius and its reciprocal

  public:
    //! Empty constructor
    Lanczos() {}

    //! Constructor
    /*!
      \param dr A D_resize object which will supply our parameters.
    */
    Lanczos(const D_resize& dr) :
      _has_radius(dr.has_support()),
      _radius(dr.support()),
      _r_radius(1.0 / _radius)
    {}

    inline double range(void) const { return _radius; }

    SAMPLE eval(double x) const throw(Uninitialised);
  };

}

#endif /* __FILTER_HH__ */
