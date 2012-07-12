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

  Image::ptr Image::convolve(Kernel2D::ptr kern) {
    std::cerr << "Convolving " << _width << "×" << _height << " image with " << kern->width() << "×" << kern->height() << " kernel..." << std::endl;
    Image::ptr out = Image::ptr(new Image(_width, _height));

    for (long int y = 0; y < _height; y++) {
      SAMPLE *outp = out->row(y);
      unsigned short int ky_start = y < kern->centrey() ? kern->centrey() - y : 0;
      unsigned short int ky_end = y > _height - kern->height() + kern->centrey() ? _height + kern->centrey() - y : kern->height();

      for (long int x = 0; x < _width; x++, outp += 3) {
	unsigned short int kx_start = x < kern->centrex() ? kern->centrex() - x : 0;
	unsigned short int kx_end = x > _width - kern->width() + kern->centrex() ? _width + kern->centrex() - x : kern->width();

	double weight = 0;
	outp[0] = outp[1] = outp[2] = 0;
	for (unsigned short int ky = ky_start; ky < ky_end; ky++) {
	  const SAMPLE *kp = kern->row(ky);
	  SAMPLE *inp = at(x + kx_start - kern->centrex(), y + ky - kern->centrey());
	  for (unsigned short int kx = kx_start; kx < kx_end; kx++, kp++, inp += 3) {
	    weight += *kp;
	    outp[0] += inp[0] * *kp;
	    outp[1] += inp[1] * *kp;
	    outp[2] += inp[2] * *kp;
	  }
	}
	if (fabs(weight) > 1e-5) {
	  weight = 1.0 / weight;
	  outp[0] *= weight;
	  outp[1] *= weight;
	  outp[2] *= weight;
	}
      }
    }

    return out;
  }

}
