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
#include <omp.h>
#include "Image.hh"
#include "Resampler.hh"

Image::Image(unsigned int w, unsigned int h) :
  _width(w),
  _height(h),
  rowdata(NULL)
{
  rowdata = (double**)malloc(_height * sizeof(double*));
#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int y = 0; y < _height; y++)
    rowdata[y] = (double*)malloc(_width * 3 * sizeof(double));
}

Image::Image(Image& other) :
  _width(other._width),
  _height(other._height),
  rowdata(NULL)
{
  rowdata = (double**)malloc(_height * sizeof(double*));
#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int y = 0; y < _height; y++) {
    rowdata[y] = (double*)malloc(_width * 3 * sizeof(double));
    memcpy(rowdata[y], other.rowdata[y], _width * 3 * sizeof(double));
  }
}

Image::~Image() {
  if (rowdata != NULL) {
#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int y = 0; y < _height; y++)
      free(rowdata[y]);
    free(rowdata);
    rowdata = NULL;
  }
  _width = _height = 0;
}

Image* Image::resize(double nw, double nh, double a) {
  if (nw < 0) {
    nw = _width * nh / _height;
  } else if (nh < 0) {
    nh = _height * nw / _width;
  }

  Image *temp, *result;
  if (nw * _height < _width * nh) {
    temp = _resize_w(nw, a);
    result = temp->_resize_h(nh, a);
  } else {
    temp = _resize_h(nh, a);
    result = temp->_resize_w(nw, a);
  }
  delete temp;

  return result;
}

Image* Image::_resize_w(double nw, double a) {
  unsigned int nwi = ceil(nw);
  Image *ni = new Image(nwi, _height);
  Resampler s(a, _width, nw);

#pragma omp parallel
  {
#pragma omp master
    {
      fprintf(stderr, "Resizing image horizontally using %d threads...\n", omp_get_num_threads());
    }
  }
#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int y = 0; y < _height; y++) {
    double *out = ni->row(y);
    for (unsigned int nx = 0; nx < nwi; nx++, out += 3) {
      unsigned int max = s.N(nx);

      out[0] = out[1] = out[2] = 0.0;
      double *weight = s.Weight(nx);
      unsigned int *x = s.Position(nx);
      for (unsigned int j = 0; j < max; j++, weight++, x++) {
	double *in = at(*x, y);

	out[0] += in[0] * *weight;
	out[1] += in[1] * *weight;
	out[2] += in[2] * *weight;
      }
    }
  }

  return ni;
}

Image* Image::_resize_h(double nh, double a) {
  unsigned int nhi = ceil(nh);
  Image *ni = new Image(_width, nhi);
  Resampler s(a, _height, nh);

#pragma omp parallel
  {
#pragma omp master
    {
      fprintf(stderr, "Resizing image vertically using %d threads...\n", omp_get_num_threads());
    }
  }
#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int ny = 0; ny < nhi; ny++) {
    unsigned int max = s.N(ny);

    double *out = ni->row(ny);
    for (unsigned int x = 0; x < _width; x++, out += 3) {
      out[0] = out[1] = out[2] = 0.0;
      double *weight = s.Weight(ny);
      unsigned int *y = s.Position(ny);
      for (unsigned int j = 0; j < max; j++, weight++, y++) {
	double *in = at(x, *y);

	out[0] += in[0] * *weight;
	out[1] += in[1] * *weight;
	out[2] += in[2] * *weight;
      }
    }
  }

  return ni;
}
