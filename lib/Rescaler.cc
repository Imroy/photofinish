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

  Lanczos::Lanczos() :
    _radius(3.0),
    _r_radius(1.0 / _radius)
  {}

  Lanczos::Lanczos(const D_resize& dr) :
    _radius(dr.support().defined() ? dr.support().get() : 3.0),
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

  void Rescaler_width::_work_on_row(ImageRow::ptr row) {
    cmsUInt32Number cmsType = _sink_header->cmsType();

    if (T_FLOAT(cmsType) == 1) {
      switch (T_BYTES(cmsType)) {
      case 1: this->_scale_row<unsigned char>(row);
	break;

      case 2: this->_scale_row<unsigned short>(row);
	break;

      case 4: this->_scale_row<unsigned int>(row);
	break;

      default:
	break;
      }
    } else {
      switch (T_BYTES(cmsType)) {
      case 0: this->_scale_row<double>(row);
	break;

      case 4: this->_scale_row<float>(row);
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

    for (unsigned int nx = 0; nx < _to_size_i; nx++, out+= cpp) {
      const SAMPLE *weight = _weights[nx];
      const P *in = &(((P*)row->data())[_start[nx] * cpp]);
      for (unsigned int j = 0; j < _size[nx]; j++, weight++, in += T_EXTRA(cmsType))
	for (unsigned char c = 0; c < T_CHANNELS(cmsType); c++, in++)
	  out[c] += (*in) * (*weight);
    }

    this->_send_image_row(row);
  }



  Rescaler_height::Rescaler_height(Function1D::ptr func, double from_start, double from_size, unsigned int from_max, double to_size) :
    Rescaler(func, from_start, from_size, from_max, to_size),
    _row_counts((unsigned int*)malloc(_to_size_i * sizeof(unsigned int)))
  {
    _rows = new ImageRow::ptr[_to_size_i];
    for (unsigned int y = 0; y < _to_size_i; y++)
      _row_counts[y] = 0;
  }

  Rescaler_height::~Rescaler_height() {
    if (_row_counts != NULL) {
      free(_row_counts);
      _row_counts = NULL;
    }
    delete [] _rows;
  }

  void Rescaler_height::receive_image_header(ImageHeader::ptr header) {
    _rescaled_header = ImageHeader::ptr(new ImageHeader(header->width(), _to_size_i));

    if (header->profile() != NULL)
      _rescaled_header->set_profile(header->profile());
    _rescaled_header->set_cmsType(header->cmsType());
    if (header->xres().defined())
      _rescaled_header->set_xres(header->xres());
    if (header->yres().defined())
      _rescaled_header->set_yres(header->yres());

    this->_send_image_header(_rescaled_header);
  }

  void Rescaler_height::receive_image_row(ImageRow::ptr row) {
    ImageSink::receive_image_row(row);
    _rows[row->y()] = row;
  }

  void Rescaler_height::_work_on_row(ImageRow::ptr row) {
    cmsUInt32Number cmsType = _sink_header->cmsType();

    if (T_FLOAT(cmsType) == 1) {
      switch (T_BYTES(cmsType)) {
      case 1: this->_scale_row<unsigned char>(row);
	break;

      case 2: this->_scale_row<unsigned short>(row);
	break;

      case 4: this->_scale_row<unsigned int>(row);
	break;

      default:
	break;
      }
    } else {
      switch (T_BYTES(cmsType)) {
      case 0: this->_scale_row<double>(row);
	break;

      case 4: this->_scale_row<float>(row);
	break;

      default:
	break;
      }
    }
  }

  template <typename P>
  void Rescaler_height::_scale_row(ImageRow::ptr row) {
    cmsUInt32Number cmsType = _sink_header->cmsType();

    unsigned int y = row->y();
    for (unsigned int ny = 0; ny < _to_size_i; ny++) {
      if ((_start[ny] <= y) && (_start[ny] + _size[ny] > y)) {
	unsigned int j = y - _start[ny];

	if (!_rows[ny])
	  _rows[ny] = ImageRow::ptr(_rescaled_header->new_row(ny));

	double weight = _weights[ny][j];
	P *in = (P*)row->data();
	P *out = (P*)_rows[ny]->data();
	for (unsigned int x = 0; x < _sink_header->width(); x++, in += T_EXTRA(cmsType), out += T_EXTRA(cmsType))
	  for (unsigned char c = 0; c < T_CHANNELS(cmsType); c++, in++, out++)
	    *out += (*in) * weight;
	_row_counts[ny]++;
	if (_row_counts[ny] == _size[ny]) {
	  _row_counts[ny] = 0;
	  this->_send_image_row(_rows[ny]);
	  _rows[ny].reset();
	}
      }
    }
  }



  FixedFactorRescaler::FixedFactorRescaler(ImageSource::ptr source, ImageSink::ptr sink, WorkGang::ptr workgang, double factor) :
    ImageModifier(source, workgang),
    _sink(sink),
    _factor(factor)
  {}

  void FixedFactorRescaler::receive_image_header(ImageHeader::ptr header) {
    double new_width = header->width() * _factor;
    double new_height = header->height() * _factor;
    Function1D::ptr lanczos(new Lanczos());

    if (new_width * header->height() < header->width() * new_height) {
      Rescaler_width *re_w = new Rescaler_width(lanczos, 0.0, header->width(), header->width(), new_width);
      _source->add_sink(ImageSink::ptr(re_w));
      _workgang->add_worker(Worker::ptr(re_w));

      Rescaler_height *re_h = new Rescaler_height(lanczos, 0.0, header->height(), header->height(), new_height);
      re_w->add_sink(ImageSink::ptr(re_h));
      _workgang->add_worker(Worker::ptr(re_h));

      re_h->add_sink(_sink);
    } else {
      Rescaler_height *re_h = new Rescaler_height(lanczos, 0.0, header->height(), header->height(), new_height);
      _source->add_sink(ImageSink::ptr(re_h));
      _workgang->add_worker(Worker::ptr(re_h));

      Rescaler_width *re_w = new Rescaler_width(lanczos, 0.0, header->width(), header->width(), new_width);
      re_h->add_sink(ImageSink::ptr(re_w));
      _workgang->add_worker(Worker::ptr(re_w));

      re_w->add_sink(_sink);
    }
  }



  FixedSizeRescaler::FixedSizeRescaler(ImageSource::ptr source, ImageSink::ptr sink, WorkGang::ptr workgang, double width, double height, bool upscale, bool inside) :
    ImageModifier(source, workgang),
    _sink(sink),
    _width(width), _height(height),
    _upscale(upscale), _inside(inside)
  {}

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

  void FixedSizeRescaler::receive_image_header(ImageHeader::ptr header) {
    double wf = _width / header->width();
    double hf = _height / header->height();

    if ((!_upscale) && (wf >= 1.0) && (hf >= 1.0)) {
      _source->add_sink(_sink);
      return;
    }

    double factor;
    if (_inside)
      factor = min(wf, hf);
    else
      factor = max(wf, hf);

    double new_width = header->width() * factor;
    double new_height = header->height() * factor;
    Function1D::ptr lanczos(new Lanczos());

    if (new_width * header->height() < header->width() * new_height) {
      Rescaler_width *re_w = new Rescaler_width(lanczos, 0.0, header->width(), header->width(), new_width);
      _source->add_sink(ImageSink::ptr(re_w));
      _workgang->add_worker(Worker::ptr(re_w));

      Rescaler_height *re_h = new Rescaler_height(lanczos, 0.0, header->height(), header->height(), new_height);
      re_w->add_sink(ImageSink::ptr(re_h));
      _workgang->add_worker(Worker::ptr(re_h));

      re_h->add_sink(_sink);
    } else {
      Rescaler_height *re_h = new Rescaler_height(lanczos, 0.0, header->height(), header->height(), new_height);
      _source->add_sink(ImageSink::ptr(re_h));
      _workgang->add_worker(Worker::ptr(re_h));

      Rescaler_width *re_w = new Rescaler_width(lanczos, 0.0, header->width(), header->width(), new_width);
      re_h->add_sink(ImageSink::ptr(re_w));
      _workgang->add_worker(Worker::ptr(re_w));

      re_w->add_sink(_sink);
    }
  }



} // namespace PhotoFinish
