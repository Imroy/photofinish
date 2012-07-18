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
#include <stdlib.h>
#include "Kernel2D.hh"
#include "Destination_items.hh"

#define sqr(x) ((x) * (x))

namespace PhotoFinish {

  Kernel2D::Kernel2D() :
    _width(0), _height(0),
    _centrex(0), _centrey(0),
    _values(NULL)
  {}

  Kernel2D::Kernel2D(short unsigned int w, short unsigned int h, short unsigned int cx, short unsigned int cy) :
    _width(w), _height(h),
    _centrex(cx), _centrey(cy),
    _values(NULL)
  {
    _values = (SAMPLE**)malloc(_height * sizeof(SAMPLE*));
    for (unsigned short int y = 0; y < _height; y++)
      _values[y] = (SAMPLE*)malloc(_width * sizeof(SAMPLE));
  }

  Kernel2D::ptr Kernel2D::create(const D_sharpen& ds) throw(DestinationError) {
    return Kernel2D::ptr(new GaussianSharpen(ds));
  }

  Kernel2D::~Kernel2D() {
    if (_values != NULL) {
      for (unsigned short int y = 0; y < _height; y++)
	free(_values[y]);
      free(_values);
      _values = NULL;
    }
  }

  Image::ptr Kernel2D::convolve(Image::ptr img) {
    std::cerr << "Convolving " << img->width() << "×" << img->height() << " image with " << _width << "×" << _height << " kernel..." << std::endl;
    Image::ptr out = Image::ptr(new Image(img->width(), img->height()));

    for (unsigned int y = 0; y < img->height(); y++) {
      SAMPLE *outp = out->row(y);
      unsigned short int ky_start = y < _centrey ? _centrey - y : 0;
      unsigned short int ky_end = y > img->height() - _height + _centrey ? img->height() + _centrey - y : _height;

      for (unsigned int x = 0; x < img->width(); x++, outp += 3) {
	unsigned short int kx_start = x < _centrex ? _centrex - x : 0;
	unsigned short int kx_end = x > img->width() - _width + _centrex ? img->width() + _centrex - x : _width;

	double weight = 0;
	outp[0] = outp[1] = outp[2] = 0;
	for (unsigned short int ky = ky_start; ky < ky_end; ky++) {
	  const SAMPLE *kp = &at(kx_start, ky);
	  SAMPLE *inp = img->at(x + kx_start - _centrex, y + ky - _centrey);
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



  GaussianSharpen::GaussianSharpen()
  {}

  GaussianSharpen::GaussianSharpen(const D_sharpen& ds) :
    Kernel2D(1 + (2 * ds.radius()), 1 + (2 * ds.radius()), ds.radius(), ds.radius()),	// Is there no better way to do this?
    _radius(ds.radius()),
    _sigma(ds.sigma())
  {
    if (!_radius.defined())
      throw Uninitialised("GaussianSharpen", "sharpen.radius");

    if (!_sigma.defined())
      throw Uninitialised("GaussianSharpen", "sharpen.sigma");

    SAMPLE total = 0;
    for (unsigned short int y = 0; y < _height; y++)
      for (unsigned short int x = 0; x < _width; x++)
	total += at(x, y) = -exp((sqr((int)x - _centrex) + sqr((int)y - _centrey)) / (-2.0 * _safe_sigma_sqr));

    at(_centrex, _centrey) = -2.0 * total;
  }

}
