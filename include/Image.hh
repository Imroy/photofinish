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
#include <type_traits>
#include <lcms2.h>

inline double lanczos(double a, double x) {
  return (a * sin(x / a) * sin(M_PI * x / a)) / (M_PI * M_PI * x * x);
}

inline double lanczos(double a, double x, double y) {
  return lanczos(a, x) * lanczos(a, y);
}

template <typename P>
class Image {
private:
  unsigned int _width, _height;
  unsigned char _channels;
  P **rowdata;
  cmsHPROFILE _profile;
  cmsUInt32Number _cmsType;

public:
  Image(unsigned int w, unsigned int h, const cmsHPROFILE p, cmsUInt32Number t) :
    _width(w),
    _height(h),
    _channels(T_CHANNELS(t) + T_EXTRA(t)),
    _profile(p),
    _cmsType(t)
  {
    rowdata = (P**)malloc(_height * sizeof(P*));
    for (unsigned int y = 0; y < _height; y++)
      rowdata[y] = (P*)malloc(_width * _channels * sizeof(P));
  }

  // In Image_png.hh
  Image(const char* filepath);

  ~Image() {
    for (unsigned int y = 0; y < _height; y++)
      free(rowdata[y]);
    free(rowdata);
    if (_profile)
      cmsCloseProfile(_profile);
  }

  inline unsigned int width(void) {
    return _width;
  }

  inline unsigned int height(void) {
    return _height;
  }

  inline unsigned char channels(void) {
    return _channels;
  }

  inline P* row(unsigned int y) {
    return &rowdata[y];
  }

  inline P* at(unsigned int x, unsigned int y) {
    return &rowdata[y][x * _channels];
  }

  inline P& at(unsigned int x, unsigned int y, unsigned char c) {
    return rowdata[y][c + (x * _channels)];
  }

  inline cmsHPROFILE profile(void) {
    return _profile;
  }

  inline cmsUInt32Number cmsType(void) {
    return _cmsType;
  }

  template <typename Q>
  inline void transform_from(Image<Q>& other, cmsUInt32Number intent, cmsUInt32Number flags) {
    if (_width != other._width) {
      fprintf(stderr, "transform_from: Image widths do not match.\n");
      exit(4);
    }
    if (_height != other._height) {
      fprintf(stderr, "transform_from: Image widths do not match.\n");
      exit(4);
    }
    cmsHTRANSFORM t = cmsCreateTransform(other._profile,
					 other._cmsType,
					 _profile,
					 _cmsType,
					 intent, flags);
    for (unsigned int y = 0; y < _height; y++)
      cmsDoTransform(t, other.rowdata[y], rowdata[y], _width);
  }
					 


};

#include "Image_png.hh"

#endif // __IMAGE_HH__
