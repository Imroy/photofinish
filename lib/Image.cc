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
#include "Image.hh"
#include <errno.h>
#include <png.h>
#include <zlib.h>
#include <lcms2.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "Image.hh"

namespace PhotoFinish {

  Image::Rows::Rows() :
    rows(0),
    rowdata(NULL)
  {}

  Image::Rows::Rows(long int rs, long int rowsize) :
    rows(rs)
  {
    rowdata = (SAMPLE**)malloc(rows * sizeof(SAMPLE*));
    for (long int r = 0; r < rows; r++)
      rowdata[r] = (SAMPLE*)malloc(rowsize * sizeof(SAMPLE));
  }

  Image::Rows::Rows(const Rows& other) :
    rowdata(other.rowdata)
  {}

  Image::Rows::~Rows() {
    if (rowdata != NULL) {
      for (long int r = 0; r < rows; r++)
	free(rowdata[r]);
      free(rowdata);
      rowdata = NULL;
    }
  }



  Image::Image() :
    _width(-1),
    _height(-1),
    _greyscale(false),
    _rows(NULL)
  {}

  Image::Image(long int w, long int h) :
    _width(w),
    _height(h),
    _greyscale(false),
    _rows(new Rows(h, w * 3))
  {}

  Image::Image(const Image& other) :
    _width(other._width),
    _height(other._height),
    _greyscale(other._greyscale),
    _rows(other._rows)
  {}

  Image::~Image() {
  }

  Image& Image::operator=(const Image& b) {
    if (this != &b) {
      _width = b._width;
      _height = b._height;
      _greyscale = b._greyscale;
      _rows = b._rows;
    }

    return *this;
  }

  void Image::copy_pixels(void) {
    Rows::ptr new_rows(new Rows(_height, _width * 3));

    for (long int y = 0; y < _height; y++)
      memcpy(new_rows->rowdata[y], _rows->rowdata[y], _width * 3 * sizeof(SAMPLE));

    _rows.swap(new_rows);
  }

}
