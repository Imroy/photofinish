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
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "Image.hh"

namespace PhotoFinish {

  Image::Image() :
    _width(-1),
    _height(-1),
    _greyscale(false),
    _rowdata(NULL)
  {}

  Image::Image(long int w, long int h) :
    _width(w),
    _height(h),
    _greyscale(false),
    _rowdata(NULL)
  {
    _rowdata = (SAMPLE**)malloc(_height * sizeof(SAMPLE*));
    for (long int y = 0; y < _height; y++)
      _rowdata[y] = (SAMPLE*)malloc(_width * 3 * sizeof(SAMPLE));
  }

  Image::~Image() {
    if (_rowdata != NULL) {
      for (long int y = 0; y < _height; y++)
	free(_rowdata[y]);
      free(_rowdata);
      _rowdata = NULL;
    }
  }

}
