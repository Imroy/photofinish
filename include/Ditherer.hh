/*
	Copyright 2014-2019 Ian Tester

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
#pragma once

#include <lcms2.h>
#include "sample.h"

namespace PhotoFinish {

  //! Class for dithering images down to 8-bit components
  class Ditherer {
  private:
    unsigned int _width;
    unsigned char _channels;
    short int *_error_row_curr, *_error_row_next;
    std::vector<unsigned char> _maxvalues;
    SAMPLE *_scale, *_unscale;

    unsigned char attemptvalue(int target, unsigned char channel);
    int actualvalue(unsigned char attempt, unsigned char channel);

  public:
    //! Constructor
    /*!
      \param width Width of the image
      \param channels Number of channels of the image
      \param maxvalues The maximum values for each channel, defaults to 255 for each
    */
    Ditherer(unsigned int width, unsigned char channels, std::vector<unsigned char> maxvalues = {});

    //! Destructor
    ~Ditherer();

    //! Base LCMS2 base type the ditherer expects the pixels to be in
    /*!
      Users of this class need to add the colour space and number of channels to this base type to be useable.
     */
    static const cmsUInt32Number cmsBaseType = BYTES_SH(2);

    //! Dither a row of image data
    /*!
      Performs a Floyd-Steinberg error diffusion dither

      \param inrow Pointer to a row of 16-bit image data
      \param outrow Pointer to a row 8-bit image data that will be produced
      \param lastrow Whether this is the last row of the image. Less has to be done.
    */
    void dither(short unsigned int *inrow, unsigned char *outrow, bool lastrow = false);
  };

}
