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
#ifndef __IMAGE_HH__
#define __IMAGE_HH__

#include <memory>
#include "sample.h"

namespace PhotoFinish {

  //! A floating-point, L*a*b* image class
  class Image {
  private:
    long int _width, _height;
    bool _greyscale;		// Used by readers and writers when converting colour spaces
    SAMPLE **_rowdata;

  public:
    typedef std::shared_ptr<Image> ptr;

    //! Empty constructor
    Image();

    //! Constructor
    /*!
      \param w,h Width and height of the image
    */
    Image(long int w, long int h);

    //! Destructor
    ~Image();

    //! Accessor
    inline const long int width(void) const { return _width; }

    //! Accessor
    inline const long int height(void) const { return _height; }

    //! Accessor
    inline const bool is_greyscale(void) const { return _greyscale; }

    //! Accessor (negative of is_greyscale)
    inline const bool is_colour(void) const { return !_greyscale; }

    //! Setter
    inline void set_greyscale(bool g = true) { _greyscale = g; }

    //! Setter (negative of set_greyscale)
    inline void set_colour(bool c = true) { _greyscale = !c; }

    //! Pointer to pixel data at start of row
    inline SAMPLE* row(long int y) const { return _rowdata[y]; }

    //! Pointer to pixel data at coordinates
    inline SAMPLE* at(long int x, long int y) const { return &(_rowdata[y][x * 3]); }

    //! Reference to pixel data at coordinates and of a given channel
    inline SAMPLE& at(long int x, long int y, unsigned char c) const { return _rowdata[y][c + (x * 3)]; }
  };

}

#endif // __IMAGE_HH__
