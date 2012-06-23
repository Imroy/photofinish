#include <stdio.h>
#include <omp.h>
#include "Image.hh"
#include "Resampler.hh"

Image::Image(unsigned int w, unsigned int h) :
  _width(w),
  _height(h),
  rowdata(NULL)
{
  rowdata = (SAMPLE**)malloc(_height * sizeof(SAMPLE*));
  for (unsigned int y = 0; y < _height; y++)
    rowdata[y] = (SAMPLE*)malloc(_width * 3 * sizeof(SAMPLE));
}

Image::Image(Image& other) :
  _width(other._width),
  _height(other._height),
  rowdata(NULL)
{
  rowdata = (SAMPLE**)malloc(_height * sizeof(SAMPLE*));
  for (unsigned int y = 0; y < _height; y++) {
    rowdata[y] = (SAMPLE*)malloc(_width * 3 * sizeof(SAMPLE));
    memcpy(rowdata[y], other.rowdata[y], _width * 3 * sizeof(SAMPLE));
  }
}

Image::~Image() {
  if (rowdata != NULL) {
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
    SAMPLE *out = ni->row(y);
    for (unsigned int nx = 0; nx < nwi; nx++, out += 3) {
      unsigned int max = s.N(nx);

      out[0] = out[1] = out[2] = 0.0;
      SAMPLE *weight = s.Weight(nx);
      unsigned int *x = s.Position(nx);
      for (unsigned int j = 0; j < max; j++, weight++, x++) {
	SAMPLE *in = at(*x, y);

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

    SAMPLE *out = ni->row(ny);
    for (unsigned int x = 0; x < _width; x++, out += 3) {
      out[0] = out[1] = out[2] = 0.0;
      SAMPLE *weight = s.Weight(ny);
      unsigned int *y = s.Position(ny);
      for (unsigned int j = 0; j < max; j++, weight++, y++) {
	SAMPLE *in = at(x, *y);

	out[0] += in[0] * *weight;
	out[1] += in[1] * *weight;
	out[2] += in[2] * *weight;
      }
    }
  }

  return ni;
}
