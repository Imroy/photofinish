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
#include "Image.hh"

namespace PhotoFinish {

  Image::Image(long int w, long int h) :
    _width(w),
    _height(h),
    rowdata(NULL)
  {
    rowdata = (SAMPLE**)malloc(_height * sizeof(SAMPLE*));
    for (long int y = 0; y < _height; y++)
      rowdata[y] = (SAMPLE*)malloc(_width * 3 * sizeof(SAMPLE));
  }

  Image::Image(const Image& other) :
    _width(other._width),
    _height(other._height),
    rowdata(NULL)
  {
    rowdata = (SAMPLE**)malloc(_height * sizeof(SAMPLE*));
    for (long int y = 0; y < _height; y++) {
      rowdata[y] = (SAMPLE*)malloc(_width * 3 * sizeof(SAMPLE));
      memcpy(rowdata[y], other.rowdata[y], _width * 3 * sizeof(SAMPLE));
    }
  }

  Image::~Image() {
    if (rowdata != NULL) {
      for (long int y = 0; y < _height; y++)
	free(rowdata[y]);
      free(rowdata);
      rowdata = NULL;
    }
    _width = _height = 0;
  }

}
