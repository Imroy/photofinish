/*
	Copyright 2014 Ian Tester

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
#include <boost/algorithm/string/predicate.hpp>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include "Benchmark.hh"
#include "Kernel1Dvar.hh"

#define sqr(x) ((x) * (x))
#define min(x,y) ((x) < (y) ? (x) : (y))

namespace PhotoFinish {

  Kernel1Dvar::Kernel1Dvar() :
    _size(nullptr), _start(nullptr),
    _weights(nullptr)
  {}

  Kernel1Dvar::Kernel1Dvar(double to_size) :
    _size(nullptr), _start(nullptr),
    _weights(nullptr),
    _to_size(to_size),
    _to_size_i(ceil(to_size))
  {
    _size = new unsigned int[_to_size_i];
    _start = new unsigned int[_to_size_i];
    _weights = new SAMPLE*[_to_size_i];
  }

  void Kernel1Dvar::build(double from_start, double from_size, unsigned int from_max) {
    _scale = from_size / _to_size;
    double range, norm_fact;
    if (_scale < 1.0) {
      range = this->range();
      norm_fact = 1.0;
    } else {
      range = this->range() * _scale;
      norm_fact = this->range() / ceil(range);
    }

#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int i = 0; i < _to_size_i; i++) {
      double centre = from_start + (i * _scale);
      unsigned int left = floor(centre - range);
      if (range > centre)
	left = 0;
      unsigned int right = ceil(centre + range);
      if (right >= from_max)
	right = from_max - 1;
      _size[i] = right + 1 - left;
      _start[i] = left;
      _weights[i] = new SAMPLE[_size[i]];
      unsigned int k = 0;
      for (unsigned int j = left; j <= right; j++, k++)
	_weights[i][k] = this->eval((centre - j) * norm_fact);

      // normalize the filter's weight's so the sum equals to 1.0, very important for avoiding box type of artifacts
      unsigned int max = _size[i];
      SAMPLE tot = 0.0;
      for (unsigned int k = 0; k < max; k++)
	tot += _weights[i][k];
      if (fabs(tot) > 1e-5) {
	tot = 1.0 / tot;
	for (unsigned int k = 0; k < max; k++)
	  _weights[i][k] *= tot;
      }
    }
  }

  Kernel1Dvar::ptr Kernel1Dvar::create(const D_resize& dr, double from_start, double from_size, unsigned int from_max, double to_size) {
    Kernel1Dvar::ptr ret;
    if (!dr.filter().defined()) {
      ret = std::make_shared<Lanczos>(D_resize::lanczos(3.0), from_start, from_size, from_max, to_size);
      return ret;
    }

    std::string filter = dr.filter();
    if (boost::iequals(filter.substr(0, min(filter.length(), 7)), "lanczos")) {
      ret = std::make_shared<Lanczos>(dr, from_start, from_size, from_max, to_size);
      return ret;
    }

    throw DestinationError("resize.filter", filter);
  }

  Kernel1Dvar::~Kernel1Dvar() {
    if (_size != nullptr) {
      delete [] _size;
      _size = nullptr;
    }

    if (_start != nullptr) {
      delete [] _start;
      _start = nullptr;
    }

    if (_weights != nullptr) {
      for (unsigned int i = 0; i < _to_size_i; i++)
	delete [] _weights[i];
      delete [] _weights;
      _weights = nullptr;
    }
  }

  // Template method that does the actual horizontal convolving
  template <typename T, int channels>
  void Kernel1Dvar::convolve_h_type_channels(Image::ptr src, Image::ptr dest, bool can_free) {
#pragma omp parallel
    {
#pragma omp master
      {
	std::cerr << "Convolving image horizontally " << src->width() << " => "
		  << std::setprecision(2) << std::fixed << dest->width()
		  << " using " << omp_get_num_threads() << " threads..." << std::endl;
      }
    }

    Timer timer;
    long long pixel_count = 0;
    timer.start();

#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int y = 0; y < src->height(); y++) {
      dest->check_row_alloc(y);
      T *out = dest->row(y)->data<T>();
      SAMPLE temp[channels];

      for (unsigned int nx = 0; nx < dest->width(); nx++) {
	for (unsigned char c = 0; c < channels; c++)
	  temp[c] = 0;
	const SAMPLE *weight = _weights[nx];
	const T *in = src->row(y)->data<T>(_start[nx]);
	for (unsigned int j = _size[nx]; j; j--, weight++) {
	  for (unsigned char c = 0; c < channels; c++, in++)
	    temp[c] += (*in) * (*weight);
	  pixel_count++;
	}
	for (unsigned char c = 0; c < channels; c++, out++)
	  *out = limitval<T>(temp[c]);
      }

      if (can_free)
	src->free_row(y);

      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tConvolved " << y + 1 << " of " << src->height() << " rows";
    }
    timer.stop();

    std::cerr << "\r\tConvolved " << src->height() << " of " << src->height() << " rows." << std::endl;

    if (benchmark_mode) {
      std::cerr << std::setprecision(2) << std::fixed;
      std::cerr << "Benchmark: Horizontally convolved " << pixel_count << " pixels in " << timer << " = " << (pixel_count / timer.elapsed() / 1e+6) << " Mpixels/second" << std::endl;
    }

  }

  // Template method that handles each type for horizontal convolving
  template <typename T>
  void Kernel1Dvar::convolve_h_type(Image::ptr src, Image::ptr dest, bool can_free) {
    unsigned char channels = src->format().total_channels();
    switch (channels) {
    case 1: // e.g greyscale
      convolve_h_type_channels<T, 1>(src, dest, can_free);
      break;

    case 2: // e.g greyscale with alpha
      convolve_h_type_channels<T, 2>(src, dest, can_free);
      break;

    case 3: // e.g RGB, Lab, etc
      convolve_h_type_channels<T, 3>(src, dest, can_free);
      break;

    case 4: // e.g CMYK, or RGB, Lab, etc with alpha
      convolve_h_type_channels<T, 4>(src, dest, can_free);
      break;

    case 5: // e.g CMYK with alpha
      convolve_h_type_channels<T, 5>(src, dest, can_free);
      break;

    case 6:
      convolve_h_type_channels<T, 6>(src, dest, can_free);
      break;

    case 7:
      convolve_h_type_channels<T, 7>(src, dest, can_free);
      break;

    case 8:
      convolve_h_type_channels<T, 8>(src, dest, can_free);
      break;

    case 9:
      convolve_h_type_channels<T, 9>(src, dest, can_free);
      break;

    case 10:
      convolve_h_type_channels<T, 10>(src, dest, can_free);
      break;

    case 11:
      convolve_h_type_channels<T, 11>(src, dest, can_free);
      break;

    case 12:
      convolve_h_type_channels<T, 12>(src, dest, can_free);
      break;

    case 13:
      convolve_h_type_channels<T, 13>(src, dest, can_free);
      break;

    case 14:
      convolve_h_type_channels<T, 14>(src, dest, can_free);
      break;

    case 15:
      convolve_h_type_channels<T, 15>(src, dest, can_free);
      break;

    default:
      std::cerr << "** Cannot handle " << (int)channels << " channels **" << std::endl;
    }
  }

  //! Convolve an image horizontally
  Image::ptr Kernel1Dvar::convolve_h(Image::ptr img, bool can_free) {
    auto ni = std::make_shared<Image>(_to_size_i, img->height(), img->format());
    ni->set_profile(img->profile());

    if (img->xres().defined())
      ni->set_xres(img->xres() / _scale);
    if (img->yres().defined())
      ni->set_yres(img->yres());

    switch (img->format().bytes_per_channel()) {
    case 1:
      convolve_h_type<unsigned char>(img, ni, can_free);
      break;

    case 2:
      convolve_h_type<short unsigned int>(img, ni, can_free);
      break;

    case 4:
      if (img->format().is_fp())
	convolve_h_type<float>(img, ni, can_free);
      else
	convolve_h_type<unsigned int>(img, ni, can_free);
      break;

    case 8:
      convolve_h_type<double>(img, ni, can_free);
      break;

    }

    return ni;
  }

  // Template method that does the actual vertical convolving
  template <typename T, int channels>
  void Kernel1Dvar::convolve_v_type_channels(Image::ptr src, Image::ptr dest, bool can_free) {
#pragma omp parallel
    {
#pragma omp master
      {
	std::cerr << "Convolving image vertically " << src->height() << " => "
		  << std::setprecision(2) << std::fixed << dest->height()
		  << " using " << omp_get_num_threads() << " threads..." << std::endl;
      }
    }

    Timer timer;
    long long pixel_count = 0;
    timer.start();

#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int ny = 0; ny < dest->height(); ny++) {
      unsigned int max = _size[ny];
      unsigned int ystart = _start[ny];

      dest->check_row_alloc(ny);
      T *out = dest->row(ny)->data<T>();
      SAMPLE temp[channels];
      for (unsigned int x = 0; x < src->width(); x++) {
	for (unsigned char c = 0; c < channels; c++)
	  temp[c] = 0;

	const SAMPLE *weight = _weights[ny];
	for (unsigned int j = 0; j < max; j++, weight++) {
	  T *in = src->row(ystart + j)->data<T>(x);
	  for (unsigned char c = 0; c < channels; c++, in++)
	    temp[c] += (*in) * (*weight);
	  pixel_count++;
	}

	for (unsigned char c = 0; c < channels; c++, out++)
	  *out = limitval<T>(temp[c]);
      }

      if (can_free)
	src->free_row(ny);

      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tConvolved " << ny + 1 << " of " << _to_size_i << " rows";
    }
    timer.stop();

    std::cerr << "\r\tConvolved " << _to_size_i << " of " << _to_size_i << " rows." << std::endl;

    if (benchmark_mode) {
      std::cerr << std::setprecision(2) << std::fixed;
      std::cerr << "Benchmark: Vertically convolved " << pixel_count << " pixels in " << timer << " = " << (pixel_count / timer.elapsed() / 1e+6) << " Mpixels/second" << std::endl;
    }

  }

  // Template method that handles each type for vertical convolving
  template <typename T>
  void Kernel1Dvar::convolve_v_type(Image::ptr src, Image::ptr dest, bool can_free) {
    unsigned char channels = src->format().total_channels();
    switch (channels) {
    case 1:
      convolve_v_type_channels<T, 1>(src, dest, can_free);
      break;

    case 2:
      convolve_v_type_channels<T, 2>(src, dest, can_free);
      break;

    case 3:
      convolve_v_type_channels<T, 3>(src, dest, can_free);
      break;

    case 4:
      convolve_v_type_channels<T, 4>(src, dest, can_free);
      break;

    case 5:
      convolve_v_type_channels<T, 5>(src, dest, can_free);
      break;

    case 6:
      convolve_v_type_channels<T, 6>(src, dest, can_free);
      break;

    case 7:
      convolve_v_type_channels<T, 7>(src, dest, can_free);
      break;

    case 8:
      convolve_v_type_channels<T, 8>(src, dest, can_free);
      break;

    case 9:
      convolve_v_type_channels<T, 9>(src, dest, can_free);
      break;

    case 10:
      convolve_v_type_channels<T, 10>(src, dest, can_free);
      break;

    case 11:
      convolve_v_type_channels<T, 11>(src, dest, can_free);
      break;

    case 12:
      convolve_v_type_channels<T, 12>(src, dest, can_free);
      break;

    case 13:
      convolve_v_type_channels<T, 13>(src, dest, can_free);
      break;

    case 14:
      convolve_v_type_channels<T, 14>(src, dest, can_free);
      break;

    case 15:
      convolve_v_type_channels<T, 15>(src, dest, can_free);
      break;

    default:
      std::cerr << "** Cannot handle " << (int)channels << " channels **" << std::endl;
    }
  }

  //! Convolve an image vertically
  Image::ptr Kernel1Dvar::convolve_v(Image::ptr img, bool can_free) {
    auto ni = std::make_shared<Image>(img->width(), _to_size_i, img->format());
    ni->set_profile(img->profile());

    if (img->xres().defined())
      ni->set_xres(img->xres());
    if (img->yres().defined())
      ni->set_yres(img->yres() / _scale);

    switch (img->format().bytes_per_channel()) {
    case 1:
      convolve_v_type<unsigned char>(img, ni, can_free);
      break;

    case 2:
      convolve_v_type<short unsigned int>(img, ni, can_free);
      break;

    case 4:
      if (img->format().is_fp())
	convolve_v_type<float>(img, ni, can_free);
      else
	convolve_v_type<unsigned int>(img, ni, can_free);
      break;

    case 8:
      convolve_v_type<double>(img, ni, can_free);
      break;

    }

    return ni;
  }



  Lanczos::Lanczos() :
    Kernel1Dvar()
  {}

    Lanczos::Lanczos(const D_resize& dr, double from_start, double from_size, unsigned int from_max, double to_size) :
      Kernel1Dvar(to_size),
      _radius(dr.support()),
      _r_radius(_radius.defined() ? 1.0 / _radius : 0.0)
  {
    build(from_start, from_size, from_max);
  }

  double Lanczos::range(void) const {
    return _radius;
  }

  SAMPLE Lanczos::eval(double x) const {
    if (!_radius.defined())
      throw Uninitialised("Lanczos", "resize.radius");

    if (fabs(x) < 1e-6)
      return 1.0;
    double pix = M_PI * x;
    return (_radius * sin(pix) * sin(pix * _r_radius)) / (sqr(M_PI) * sqr(x));
  }

}
