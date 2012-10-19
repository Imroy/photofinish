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
#include "Rescaler.hh"

namespace PhotoFinish {

  Lanczos::Lanczos(D_resize::ptr dr) :
    _radius(dr->support().defined() ? dr->support().get() : 3.0),
    _r_radius(1.0 / _radius)
  {}



  Rescaler::Rescaler(Function1D::ptr func, double from_start, double from_size, unsigned int from_max, double to_size) :
    _size(NULL), _start(NULL),
    _weights(NULL),
    _scale(0),
    _to_size(to_size),
    _to_size_i(ceil(to_size))
  {
    _size = (unsigned int*)malloc(_to_size_i * sizeof(unsigned int));
    _start = (unsigned int*)malloc(_to_size_i * sizeof(unsigned int));
    _weights = (SAMPLE**)malloc(_to_size_i * sizeof(SAMPLE*));

    _scale = from_size / _to_size;
    double range, norm_fact;
    if (_scale < 1.0) {
      range = _func->range();
      norm_fact = 1.0;
    } else {
      range = _func->range() * _scale;
      norm_fact = _func->range() / ceil(range);
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
      _weights[i] = (SAMPLE*)malloc(_size[i] * sizeof(SAMPLE));
      unsigned int k = 0;
      for (unsigned int j = left; j <= right; j++, k++)
	_weights[i][k] = _func->eval((centre - j) * norm_fact);

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

  Rescaler::~Rescaler() {
    if (_size != NULL) {
      free(_size);
      _size = NULL;
    }

    if (_start != NULL) {
      free(_start);
      _start = NULL;
    }

    if (_weights != NULL) {
      for (unsigned int i = 0; i < _to_size_i; i++)
	free(_weights[i]);
      free(_weights);
      _weights = NULL;
    }
  }



  Rescaler_width::Rescaler_width(Function1D::ptr func, double from_start, double from_size, unsigned int from_max, double to_size) :
    Rescaler(func, from_start, from_size, from_max, to_size)
  {}

  void Rescaler_width::receive_image_header(ImageHeader::ptr header) {
    _rescaled_header = ImageHeader::ptr(new ImageHeader(_to_size_i, header->height()));

    if (header->profile() != NULL)
      _rescaled_header->set_profile(header->profile());
    _rescaled_header->set_cmsType(header->cmsType());
    if (header->xres().defined())
      _rescaled_header->set_xres(header->xres());
    if (header->yres().defined())
      _rescaled_header->set_yres(header->yres());

    this->_send_image_header(_rescaled_header);
  }

  void Rescaler_width::receive_image_row(ImageRow::ptr row) {
    cmsUInt32Number cmsType = _sink_header->cmsType();

    if (T_FLOAT(cmsType) == 1) {
      switch (T_BYTES(cmsType)) {
      case 1:
	this->_scale_row<unsigned char>(row);
	break;

      case 2:
	this->_scale_row<unsigned short>(row);
	break;

      case 4:
	this->_scale_row<unsigned int>(row);
	break;

      default:
	break;
      }
    } else {
      switch (T_BYTES(cmsType)) {
      case 0:
	this->_scale_row<double>(row);
	break;

      case 4:
	this->_scale_row<float>(row);
	break;

      default:
	break;
      }
    }
  }

  template <typename P>
  void Rescaler_width::_scale_row(ImageRow::ptr row) {
    cmsUInt32Number cmsType = _sink_header->cmsType();
    unsigned int cpp = T_CHANNELS(cmsType) + T_EXTRA(cmsType);
    ImageRow::ptr new_row(_rescaled_header->new_row(row->y()));
    P *out = (P*)new_row->data();

    for (unsigned int nx = 0; nx < _to_size_i; nx++, out += 3) {
      unsigned int max = _size[nx];

      out[0] = out[1] = out[2] = 0.0;
      const SAMPLE *weight = _weights[nx];
      const P *in = &(((P*)row->data())[_start[nx] * cpp]);
      for (unsigned int j = 0; j < max; j++, weight++)
	for (unsigned char c = 0; c < T_CHANNELS(cmsType); c++, in++)
	  out[c] += (*in) * (*weight);
    }

    this->_send_image_row(row);
  }

  void Rescaler_width::receive_image_end(void) {
    this->_send_image_end();
  }

} // namespace PhotoFinish
