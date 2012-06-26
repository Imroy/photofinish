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

#include <stdlib.h>
#include <math.h>
#include <string.h>

// A floating-point, L*a*b* image class
class Image {
private:
  unsigned int _width, _height;
  SAMPLE **rowdata;

public:
  Image(unsigned int w, unsigned int h);
  Image(Image& other);
  ~Image();

  // Methods for accessing the private data
  inline unsigned int width(void) const {
    return _width;
  }

  inline unsigned int height(void) const {
    return _height;
  }

  inline SAMPLE* row(unsigned int y) const {
    return rowdata[y];
  }

  inline SAMPLE* at(unsigned int x, unsigned int y) const {
    return &rowdata[y][x * 3];
  }

  inline SAMPLE& at(unsigned int x, unsigned int y, unsigned char c) const {
    return rowdata[y][c + (x * 3)];
  }

private:

};

#endif // __IMAGE_HH__
