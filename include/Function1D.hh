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
#ifndef __FUNCTION1D_HH__
#define __FUNCTION1D_HH__

#include <memory>

namespace PhotoFinish {

  //! Abstract base class
  class Function1D {
  public:
    //! Empty onstructor
    inline Function1D() {}

    //! The size of this filter
    virtual double range(void) const = 0;

    //! Evaluate the filter at a given point
    virtual double eval(double x) const = 0;

    typedef std::shared_ptr<Function1D> ptr;
  }; // class Function1D

} // namespace PhotoFinish

#endif // __FUNCTION1D_HH__
