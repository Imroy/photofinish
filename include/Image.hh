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
#include "Definable.hh"
#include "sample.h"

namespace PhotoFinish {

  //! A floating-point, L*a*b* image class
  class Image {
  private:
    unsigned int _width, _height;
    bool _greyscale;		// Used by readers and writers when converting colour spaces
    SAMPLE **_rowdata;
    definable<double> _xres, _yres;		// PPI

    inline void _check_rowdata_alloc(unsigned int y) {
      if (_rowdata[y] == NULL)
	_rowdata[y] = (SAMPLE*)malloc(_width * 3 * sizeof(SAMPLE));
    }

  public:
    //! Shared pointer for an Image
    typedef std::shared_ptr<Image> ptr;

    //! Empty constructor
    Image();

    //! Constructor
    /*!
      \param w,h Width and height of the image
    */
    Image(unsigned int w, unsigned int h);

    //! Destructor
    ~Image();

    //! The width of this image
    inline const unsigned int width(void) const { return _width; }

    //! The height of this image
    inline const unsigned int height(void) const { return _height; }

    //! Is this image greyscale?
    inline const bool is_greyscale(void) const { return _greyscale; }

    //! Is this image colour? i.e opposite of is_greyscale()
    inline const bool is_colour(void) const { return !_greyscale; }

    //! Set this image as greyscale
    inline void set_greyscale(bool g = true) { _greyscale = g; }

    //! Set this image as colour i.e opposite of set_greyscale
    inline void set_colour(bool c = true) { _greyscale = !c; }

    //! The X resolution of this image (PPI)
    inline const definable<double> xres(void) const { return _xres; }

    //! The Y resolution of this image (PPI)
    inline const definable<double> yres(void) const { return _yres; }

    //! Set both the X and Y resolution (PPI)
    inline void set_resolution(double r) { _xres = _yres = r; }

    //! Set the X resolution (PPI)
    inline void set_xres(double r) { _xres = r; }

    //! Set the Y resolution (PPI)
    inline void set_yres(double r) { _yres = r; }

    //! Set the X and Y resolutions (PPI)
    inline void set_resolution(double xr, double yr) { _xres = xr; _yres = yr; }

    //! Set the resolution given the length of the longest side (in inches)
    inline void set_resolution_from_size(double size) { _xres = _yres = (_width > _height ? _width : _height) / size; }

    //! Pointer to pixel data at start of row
    inline SAMPLE* row(unsigned int y) {
      _check_rowdata_alloc(y);
      return _rowdata[y];
    }

    //! Pointer to pixel data at coordinates
    inline SAMPLE* at(unsigned int x, unsigned int y) {
      _check_rowdata_alloc(y);
      return &(_rowdata[y][x * 3]);
    }

    //! Reference to pixel data at coordinates and of a given channel
    inline SAMPLE& at(unsigned int x, unsigned int y, unsigned char c) {
      _check_rowdata_alloc(y);
      return _rowdata[y][c + (x * 3)];
    }

    //! Free the memory storing row 'y'
    inline void free_row(unsigned int y) {
      if (_rowdata[y] != NULL) {
	free(_rowdata[y]);
	_rowdata[y] = NULL;
      }
    }

  };

}

#endif // __IMAGE_HH__
