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
  class Base_Filter {
  protected:
    double _radius;

  public:
    //! Constructor
    /*! Radius defaults to '3'. */
    Base_Filter() :
      _radius(3)
    {}

    //! Constructor
    /*!
      \param dr A D_resize object which will supply our parameters.
    */
    Base_Filter(const D_resize& dr) :
      _radius(dr.has_support() ? dr.support() : 3)
    {}

    //! Accessor
    inline double radius(void) const { return _radius; }

    //! Evaluate the filter at a given point
    virtual SAMPLE eval(double x) const = 0;
  };

  //! Lanczos filter
  class Lanczos : public Base_Filter {
  private:
    double _r_radius;	// Reciprocal of the radius

  public:
    //! Constructor
    /*!
      \param dr A D_resize object which will supply our parameters.
    */
    Lanczos(const D_resize& dr) :
      Base_Filter(dr),
      _r_radius(1.0 / _radius)
    {}

    SAMPLE eval(double x) const;
  };

  //! Filter factory/wrapper class
  class Filter : public Base_Filter {
  private:
    typedef std::shared_ptr<Base_Filter> ptr;

    ptr _filter;

  public:
    //! Empty constructor
    Filter();

    //! Constructor
    /*!
      \param resize A D_resize object which will supply our parameters.
    */
    Filter(const D_resize& resize) throw(DestinationError);

    inline SAMPLE eval(double x) const {
      if (_filter == NULL)
	throw Uninitialised("Filter");
      return _filter->eval(x);
    }
  };

}

#endif /* __FILTER_HH__ */
