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
#include <omp.h>
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

  Kernel2D::Kernel2D(short unsigned int size, short unsigned int centre) :
    _width(size), _height(size),
    _centrex(centre), _centrey(centre),
    _values(NULL)
  {
    _values = (SAMPLE**)malloc(_height * sizeof(SAMPLE*));
    for (unsigned short int y = 0; y < _height; y++)
      _values[y] = (SAMPLE*)malloc(_width * sizeof(SAMPLE));
  }

  Kernel2D::ptr Kernel2D::create(const D_sharpen& ds) throw(DestinationError) {
    return std::make_shared<GaussianSharpen>(ds);
  }

  Kernel2D::~Kernel2D() {
    if (_values != NULL) {
      for (unsigned short int y = 0; y < _height; y++)
	free(_values[y]);
      free(_values);
      _values = NULL;
    }
  }

  template <typename T>
  void Kernel2D::do_convolve(Image::ptr src, Image::ptr dest, bool can_free) {
    unsigned char channels = T_CHANNELS(src->type()) + T_EXTRA(src->type());

    int *row_needs;
    if (can_free) {
      int num_threads = omp_get_num_threads();
      row_needs = (int*)malloc(src->height() * sizeof(int));
      for (unsigned int y = 0; y < src->height(); y++)
	row_needs[y] = num_threads;
    }
    unsigned int next_freed = 0;
    omp_lock_t freed_lock;
    omp_init_lock(&freed_lock);
#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int y = 0; y < src->height(); y++) {
      T *outp = (T*)dest->row(y);
      short unsigned int ky_start = y < _centrey ? _centrey - y : 0;
      short unsigned int ky_end = y > src->height() - _height + _centrey ? src->height() + _centrey - y : _height;

      for (unsigned int x = 0; x < src->width(); x++, outp += channels) {
	short unsigned int kx_start = x < _centrex ? _centrex - x : 0;
	short unsigned int kx_end = x > src->width() - _width + _centrex ? src->width() + _centrex - x : _width;

	double weight = 0;
	for (unsigned char c = 0; c < channels; c++)
	  outp[c] = 0;

	for (short unsigned int ky = ky_start; ky < ky_end; ky++) {
	  const SAMPLE *kp = _values[ky] + kx_start;
	  T *inp = (T*)src->at(x + kx_start - _centrex, y + ky - _centrey);
	  for (short unsigned int kx = kx_start; kx < kx_end; kx++, kp++) {
	    weight += *kp;
	    for (unsigned char c = 0; c < channels; c++, inp++)
	      outp[c] += (*inp) * (*kp);
	  }
	}
	if (fabs(weight) > 1e-5) {
	  weight = 1.0 / weight;
	  for (unsigned char c = 0; c < channels; c++)
	    outp[c] *= weight;
	}
      }
      if (can_free && (y > _centrey)) {
	omp_set_lock(&freed_lock);
	row_needs[y + ky_start - _centrey]--;
	for (;row_needs[next_freed] == 0; next_freed++)
	  src->free_row(next_freed);
	omp_unset_lock(&freed_lock);
      }

      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tConvolved " << y + 1 << " of " << src->height() << " rows";
    }
    std::cerr << "\r\tConvolved " << src->height() << " of " << src->height() << " rows." << std::endl;
    if (can_free) {
      free(row_needs);
      for (; next_freed < src->height(); next_freed++)
	src->free_row(next_freed);
    }
  }

  Image::ptr Kernel2D::convolve(Image::ptr img, bool can_free) {
#pragma omp parallel
    {
#pragma omp master
      {
	std::cerr << "Convolving " << img->width() << "×" << img->height()
		  << " image with " << _width << "×" << _height
		  << " kernel using " << omp_get_num_threads() << " threads..." << std::endl;
      }
    }
    auto out = std::make_shared<Image>(img->width(), img->height(), img->type());

    if (img->xres().defined())
      out->set_xres(img->xres());
    if (img->yres().defined())
      out->set_yres(img->yres());

    switch (T_BYTES_REAL(img->type())) {
    case 1:
      do_convolve<unsigned char>(img, out, can_free);
      break;

    case 2:
      do_convolve<short unsigned int>(img, out, can_free);
      break;

    case 4:
      if (T_FLOAT(img->type()))
	do_convolve<float>(img, out, can_free);
      else
	do_convolve<unsigned int>(img, out, can_free);
      break;

    case 8:
      if (T_FLOAT(img->type()))
	do_convolve<double>(img, out, can_free);
      else
	do_convolve<unsigned long>(img, out, can_free);
      break;

    }

    return out;
  }



  GaussianSharpen::GaussianSharpen()
  {}

  GaussianSharpen::GaussianSharpen(const D_sharpen& ds) :
    Kernel2D(1 + (2 * ceil(ds.radius())), ceil(ds.radius())),
    _radius(ds.radius()),
    _sigma(ds.sigma()),
    _safe_sigma_sqr(_sigma.defined() && (fabs(_sigma) > 1e-5) ? sqr(_sigma) : 1e-5 )
  {
    if (!_radius.defined())
      throw Uninitialised("GaussianSharpen", "sharpen.radius");

    if (!_sigma.defined())
      throw Uninitialised("GaussianSharpen", "sharpen.sigma");

    SAMPLE total = 0;
#pragma omp parallel for schedule(dynamic, 1) shared(total)
    for (short unsigned int y = 0; y < _height; y++)
      for (short unsigned int x = 0; x < _width; x++)
	total += _values[y][x] = -exp((sqr((int)x - _centrex) + sqr((int)y - _centrey)) / (-2.0 * _safe_sigma_sqr));

    _values[_centrey][_centrex] = -2.0 * total;
  }

}
