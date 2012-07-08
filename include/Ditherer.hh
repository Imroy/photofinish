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
#ifndef __DITHERER_HH__
#define __DITHERER_HH__

namespace PhotoFinish {

  //! Class for dithering images down to 8-bit components
  class Ditherer {
  private:
    long int width;
    unsigned char channels;
    short int **error_rows;
    int curr_row, next_row;

  public:
    //! Constructor
    /*!
      \param w Width of the image
      \param c Channels of the image
    */
    Ditherer(long int w, unsigned char c);

    //! Destructor
    ~Ditherer();

    //! Dither a row of image data
    /*!
      Performs a Floyd-Steinberg error diffusion dither

      \param inrow Pointer to a row of 16-bit image data
      \param outrow Pointer to a row 8-bit image data that will be produced
      \param lastrow Wether this is the last row of the image. Less has to be done.
    */
    void dither(short unsigned int *inrow, unsigned char *outrow, bool lastrow = false);
  };

}

#endif /* __DITHERER_HH__ */
