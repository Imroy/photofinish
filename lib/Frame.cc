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
#include <omp.h>
#include "Frame.hh"
#include "Destination_items.hh"
#include "Resampler.hh"

namespace PhotoFinish {

  Frame::Frame(double tw, double th, double x, double y, double w, double h, double r) :
    D_target(std::string(""), tw, th),
    _crop_x(x), _crop_y(y),
    _crop_w(w), _crop_h(h),
    _resolution(r)
  {}

  Frame::Frame(const D_target& target, double x, double y, double w, double h, double r) :
    D_target(target),
    _crop_x(x), _crop_y(y),
    _crop_w(w), _crop_h(h),
    _resolution(r)
  {}

  // Private functions for 1-dimensional scaling
  Image::ptr _crop_resize_w(Image::ptr img, const Filter& filter, double x, double cw, double nw) {
    long int nwi = ceil(nw);
    Image::ptr ni(new Image(nwi, img->height()));
    Resampler s(filter, x, cw, img->width(), nw);

#pragma omp parallel
    {
#pragma omp master
      {
	fprintf(stderr, "Resizing image horizontally using %d threads...\n", omp_get_num_threads());
      }
    }
#pragma omp parallel for schedule(dynamic, 1)
    for (long int y = 0; y < img->height(); y++) {
      SAMPLE *out = ni->row(y);
      for (long int nx = 0; nx < nwi; nx++, out += 3) {
	long int max = s.N(nx);

	out[0] = out[1] = out[2] = 0.0;
	const SAMPLE *weight = s.Weight(nx);
	const long int *x = s.Position(nx);
	for (long int j = 0; j < max; j++, weight++, x++) {
	  SAMPLE *in = img->at(*x, y);

	  out[0] += in[0] * *weight;
	  out[1] += in[1] * *weight;
	  out[2] += in[2] * *weight;
	}
      }
    }

    ni->set_greyscale(img->is_greyscale());
    return ni;
  }

  Image::ptr _crop_resize_h(Image::ptr img, const Filter& filter, double y, double ch, double nh) {
    long int nhi = ceil(nh);
    Image::ptr ni(new Image(img->width(), nhi));
    Resampler s(filter, y, ch, img->height(), nh);

#pragma omp parallel
    {
#pragma omp master
      {
	fprintf(stderr, "Resizing image vertically using %d threads...\n", omp_get_num_threads());
      }
    }
#pragma omp parallel for schedule(dynamic, 1)
    for (long int ny = 0; ny < nhi; ny++) {
      long int max = s.N(ny);

      SAMPLE *out = ni->row(ny);
      for (long int x = 0; x < img->width(); x++, out += 3) {
	out[0] = out[1] = out[2] = 0.0;
	const SAMPLE *weight = s.Weight(ny);
	const long int *y = s.Position(ny);
	for (long int j = 0; j < max; j++, weight++, y++) {
	  SAMPLE *in = img->at(x, *y);

	  out[0] += in[0] * *weight;
	  out[1] += in[1] * *weight;
	  out[2] += in[2] * *weight;
	}
      }
    }

    ni->set_greyscale(img->is_greyscale());
    return ni;
  }

  Image::ptr Frame::crop_resize(Image::ptr img, const Filter& filter) {
    if (_width * img->height() < img->width() * _height) {
      Image::ptr temp = _crop_resize_w(img, filter, _crop_x, _crop_w, _width);
      return _crop_resize_h(temp, filter, _crop_y, _crop_h, _height);
    }

    Image::ptr temp = _crop_resize_h(img, filter, _crop_y, _crop_h, _height);
    return _crop_resize_w(temp, filter, _crop_x, _crop_w, _width);
  }

}
