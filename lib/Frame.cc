#include <omp.h>
#include "Destination.hh"
#include "Resampler.hh"

Frame::Frame(const D_target& target, double x, double y, double w, double h, double r) :
  D_target(target),
  _crop_x(x), _crop_y(y),
  _crop_w(w), _crop_h(h),
  _resolution(r)
{}

Frame::~Frame() {
}

// Private functions for 1-dimensional scaling
Image* _crop_resize_w(Image* img, _Filter* filter, double x, double cw, double nw) {
  unsigned int nwi = ceil(nw);
  Image *ni = new Image(nwi, img->height());
  Resampler s(filter, x, cw, img->width(), nw);

#pragma omp parallel
  {
#pragma omp master
    {
      fprintf(stderr, "Resizing image horizontally using %d threads...\n", omp_get_num_threads());
    }
  }
#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int y = 0; y < img->height(); y++) {
    SAMPLE *out = ni->row(y);
    for (unsigned int nx = 0; nx < nwi; nx++, out += 3) {
      unsigned int max = s.N(nx);

      out[0] = out[1] = out[2] = 0.0;
      SAMPLE *weight = s.Weight(nx);
      unsigned int *x = s.Position(nx);
      for (unsigned int j = 0; j < max; j++, weight++, x++) {
	SAMPLE *in = img->at(*x, y);

	out[0] += in[0] * *weight;
	out[1] += in[1] * *weight;
	out[2] += in[2] * *weight;
      }
    }
  }

  return ni;
}

Image* _crop_resize_h(Image* img, _Filter* filter, double y, double ch, double nh) {
  unsigned int nhi = ceil(nh);
  Image *ni = new Image(img->width(), nhi);
  Resampler s(filter, y, ch, img->height(), nh);

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
    for (unsigned int x = 0; x < img->width(); x++, out += 3) {
      out[0] = out[1] = out[2] = 0.0;
      SAMPLE *weight = s.Weight(ny);
      unsigned int *y = s.Position(ny);
      for (unsigned int j = 0; j < max; j++, weight++, y++) {
	SAMPLE *in = img->at(x, *y);

	out[0] += in[0] * *weight;
	out[1] += in[1] * *weight;
	out[2] += in[2] * *weight;
      }
    }
  }

  return ni;
}

Image* Frame::crop_resize(Image* img, _Filter* filter) {
  Image *temp, *result;
  if (_width * img->height() < img->width() * _height) {
    temp = _crop_resize_w(img, filter, _crop_x, _crop_w, _width);
    result = _crop_resize_h(temp, filter, _crop_y, _crop_h, _height);
  } else {
    temp = _crop_resize_h(img, filter, _crop_y, _crop_h, _height);
    result = _crop_resize_w(temp, filter, _crop_x, _crop_w, _width);
  }
  delete temp;

  return result;
}

