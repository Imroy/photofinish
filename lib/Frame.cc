#include <omp.h>
#include "Frame.hh"
#include "Destination_items.hh"
#include "Resampler.hh"

namespace PhotoFinish {

  Frame::Frame(const D_target& target, double x, double y, double w, double h, double r) :
    D_target(target),
    _crop_x(x), _crop_y(y),
    _crop_w(w), _crop_h(h),
    _resolution(r)
  {}

  Frame::~Frame() {
  }

  // Private functions for 1-dimensional scaling
  const Image& _crop_resize_w(const Image& img, const Filter& filter, double x, double cw, double nw) {
    long int nwi = ceil(nw);
    Image *ni = new Image(nwi, img.height());
    Resampler s(filter, x, cw, img.width(), nw);

#pragma omp parallel
    {
#pragma omp master
      {
	fprintf(stderr, "Resizing image horizontally using %d threads...\n", omp_get_num_threads());
      }
    }
#pragma omp parallel for schedule(dynamic, 1)
    for (long int y = 0; y < img.height(); y++) {
      SAMPLE *out = ni->row(y);
      for (long int nx = 0; nx < nwi; nx++, out += 3) {
	long int max = s.N(nx);

	out[0] = out[1] = out[2] = 0.0;
	SAMPLE *weight = s.Weight(nx);
	long int *x = s.Position(nx);
	for (long int j = 0; j < max; j++, weight++, x++) {
	  SAMPLE *in = img.at(*x, y);

	  out[0] += in[0] * *weight;
	  out[1] += in[1] * *weight;
	  out[2] += in[2] * *weight;
	}
      }
    }

    return *ni;
  }

  const Image& _crop_resize_h(const Image& img, const Filter& filter, double y, double ch, double nh) {
    long int nhi = ceil(nh);
    Image *ni = new Image(img.width(), nhi);
    Resampler s(filter, y, ch, img.height(), nh);

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
      for (long int x = 0; x < img.width(); x++, out += 3) {
	out[0] = out[1] = out[2] = 0.0;
	SAMPLE *weight = s.Weight(ny);
	long int *y = s.Position(ny);
	for (long int j = 0; j < max; j++, weight++, y++) {
	  SAMPLE *in = img.at(x, *y);

	  out[0] += in[0] * *weight;
	  out[1] += in[1] * *weight;
	  out[2] += in[2] * *weight;
	}
      }
    }

    return *ni;
  }

  const Image& Frame::crop_resize(const Image& img, const Filter& filter) {
    if (_width * img.height() < img.width() * _height) {
      Image temp = _crop_resize_w(img, filter, _crop_x, _crop_w, _width);
      return _crop_resize_h(temp, filter, _crop_y, _crop_h, _height);
    }

    Image temp = _crop_resize_h(img, filter, _crop_y, _crop_h, _height);
    return _crop_resize_w(temp, filter, _crop_x, _crop_w, _width);
  }

}
