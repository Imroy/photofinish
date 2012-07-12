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
#ifndef __RESAMPLER_HH__
#define __RESAMPLER_HH__

#include <stdlib.h>
#include <math.h>
#include "Filter.hh"
#include "sample.h"

namespace PhotoFinish {

  //! Creates and stores coefficients for cropping and resizing an image
  class Resampler {
  private:
    long int *_N, **_Position;
    SAMPLE **_Weight;
    long int _to_size_i;

  public:
    //! Constructor
    /*!
      \param filter Supplies the basis function for the resampling
      \param from_start The starting point of the crop/resample
      \param from_size The size of the crop/resample
      \param from_max The size (maximum dimenstion) of the input
      \param to_size The size of the output
    */
    Resampler(Filter::ptr filter, double from_start, double from_size, long int from_max, double to_size);

    //! Destructor
    ~Resampler();

    //! Accessor for the size of the other two arrays
    inline const long int& N(long int i) const { return _N[i]; }

    //! Accessor for the Position array
    inline const long int* Position(long int i) const { return _Position[i]; }

    //! Accessor for the Position array
    inline const long int& Position(long int f, long int i) const { return _Position[i][f]; }

    //! Accessor for the Weight array
    inline const SAMPLE* Weight(long int i) const { return _Weight[i]; }

    //! Accessor for the Weight array
    inline const SAMPLE& Weight(long int f, long int i) const { return _Weight[i][f]; }

  };

}

#endif // __RESAMPLER_HH__
