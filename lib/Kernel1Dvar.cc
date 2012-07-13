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
#include <iomanip>
#include <boost/algorithm/string.hpp>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include "Kernel1Dvar.hh"

#define sqr(x) ((x) * (x))
#define min(x,y) ((x) < (y) ? (x) : (y))

namespace PhotoFinish {

  Kernel1Dvar::Kernel1Dvar() :
    _size(NULL), _start(NULL),
    _weights(NULL)
  {}

  Kernel1Dvar::Kernel1Dvar(double to_size) :
    _size(NULL), _start(NULL),
    _weights(NULL),
    _to_size(to_size),
    _to_size_i(ceil(to_size))
  {
    _size = (long int*)malloc(_to_size_i * sizeof(long int));
    _start = (long int*)malloc(_to_size_i * sizeof(long int));
    _weights = (SAMPLE**)malloc(_to_size_i * sizeof(SAMPLE*));
  }

  void Kernel1Dvar::build(const D_resize& dr, double from_start, double from_size, long int from_max) throw(DestinationError) {
    double scale = from_size / _to_size;
    double range, norm_fact;
    if (scale < 1.0) {
      range = this->range();
      norm_fact = 1.0;
    } else {
      range = this->range() * scale;
      norm_fact = this->range() / ceil(range);
    }

#pragma omp parallel for schedule(dynamic, 1)
    for (long int i = 0; i < _to_size_i; i++) {
      double centre = from_start + (i * scale);
      int left = floor(centre - range);
      if (left < 0)
	left = 0;
      long int right = ceil(centre + range);
      if (right >= from_max)
	right = from_max - 1;
      _size[i] = right - left + 1;
      _start[i] = left;
      _weights[i] = (SAMPLE*)malloc(_size[i] * sizeof(SAMPLE));
      long int k = 0;
      for (long int j = left; j <= right; j++, k++)
	_weights[i][k] = this->eval((centre - j) * norm_fact);
      // normalize the filter's weight's so the sum equals to 1.0, very important for avoiding box type of artifacts
      long int max = _size[i];
      SAMPLE tot = 0.0;
      for (long int k = 0; k < max; k++)
	tot += _weights[i][k];
      if (tot != 0) // 0 should never happen except bug in filter
	for (long int k = 0; k < max; k++)
	  _weights[i][k] /= tot;
    }

  }

  Kernel1Dvar::ptr Kernel1Dvar::create(const D_resize& dr, double from_start, double from_size, long int from_max, double to_size) throw(DestinationError) {
    Kernel1Dvar::ptr ret;
    if (!dr.has_filter()) {
      ret = Kernel1Dvar::ptr(new Lanczos(D_resize::lanczos(3.0), from_start, from_size, from_max, to_size));
      return ret;
    }

    std::string filter = dr.filter();
    if (boost::iequals(filter.substr(0, min(filter.length(), 7)), "lanczos")) {
      ret = Kernel1Dvar::ptr(new Lanczos(dr, from_start, from_size, from_max, to_size));
      return ret;
    }

    throw DestinationError("resize.filter", filter);
  }

  Kernel1Dvar::~Kernel1Dvar() {
    if (_size != NULL) {
      free(_size);
      _size = NULL;
    }

    if (_start != NULL) {
      free(_start);
      _start = NULL;
    }

    if (_weights != NULL) {
      for (long int i = 0; i < _to_size_i; i++)
	free(_weights[i]);
      free(_weights);
      _weights = NULL;
    }
  }

  //! Convolve an image horizontally
  Image::ptr Kernel1Dvar::convolve_h(Image::ptr img) {
    Image::ptr ni(new Image(_to_size_i, img->height()));

#pragma omp parallel
    {
#pragma omp master
      {
	std::cerr << "Convolving image horizontally " << img->width() << " => "
		  << std::setprecision(2) << std::fixed << _to_size_i
		  << " using " << omp_get_num_threads() << " threads..." << std::endl;
      }
    }
#pragma omp parallel for schedule(dynamic, 1)
    for (long int y = 0; y < img->height(); y++) {
      SAMPLE *out = ni->row(y);

      for (long int nx = 0; nx < _to_size_i; nx++, out += 3) {
	long int max = size(nx);

	out[0] = out[1] = out[2] = 0.0;
	const SAMPLE *weight = this->row(nx);
	const SAMPLE *in = img->at(this->start(nx), y);
	for (long int j = 0; j < max; j++, weight++, in += 3) {
	  out[0] += in[0] * *weight;
	  out[1] += in[1] * *weight;
	  out[2] += in[2] * *weight;
	}
      }
    }

    ni->set_greyscale(img->is_greyscale());
    return ni;
  }

  //! Convolve an image vertically
  Image::ptr Kernel1Dvar::convolve_v(Image::ptr img) {
   Image::ptr ni(new Image(img->width(), _to_size_i));

#pragma omp parallel
    {
#pragma omp master
      {
	std::cerr << "Convolving image vertically " << img->height() << " => "
		  << std::setprecision(2) << std::fixed << _to_size_i
		  << " using " << omp_get_num_threads() << " threads..." << std::endl;
      }
    }
#pragma omp parallel for schedule(dynamic, 1)
    for (long int ny = 0; ny < _to_size_i; ny++) {
      long int max = size(ny);
      long int ystart = this->start(ny);
      long int j = 0;
      const SAMPLE *weight = &this->at(j, ny);

      SAMPLE *in = img->row(ystart);
      SAMPLE *out = ni->row(ny);
      for (long int x = 0; x < img->width(); x++, in += 3, out += 3) {
	out[0] = in[0] * *weight;
	out[1] = in[1] * *weight;
	out[2] = in[2] * *weight;
      }

      weight++;
      for (j = 1; j < max; j++, weight++) {
	in = img->row(ystart + j);
	out = ni->row(ny);
	for (long int x = 0; x < img->width(); x++, in += 3, out += 3) {
	  out[0] += in[0] * *weight;
	  out[1] += in[1] * *weight;
	  out[2] += in[2] * *weight;
	}
      }
    }

    ni->set_greyscale(img->is_greyscale());
    return ni;
  }



  Lanczos::Lanczos() :
    Kernel1Dvar(),
    _has_radius(false)
  {}

    Lanczos::Lanczos(const D_resize& dr, double from_start, double from_size, long int from_max, double to_size) :
      Kernel1Dvar(to_size),
      _has_radius(dr.has_support()),
      _radius(dr.support()),
      _r_radius(1.0 / _radius)
  {
    build(dr, from_start, from_size, from_max);
  }

  double Lanczos::range(void) const {
    return _radius;
  }

  SAMPLE Lanczos::eval(double x) const throw(Uninitialised) {
    if (!_has_radius)
      throw Uninitialised("Lanczos", "resize.radius");

    if (fabs(x) < 1e-6)
      return 1.0;
    double pix = M_PI * x;
    return (_radius * sin(pix) * sin(pix * _r_radius)) / (sqr(M_PI) * sqr(x));
  }

}
