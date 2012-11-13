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
#include "Sharpen.hh"
#include "Destination_items.hh"

namespace PhotoFinish {

  Gaussian::Gaussian() :
    _radius(1.0),
    _sigma(10.0),
    _safe_sigma_sqr(100.0)
  {}

  Gaussian::Gaussian(const D_sharpen& ds) :
    _radius(ds.radius().defined() ? ds.radius().get() : 1.0),
    _sigma(ds.sigma().defined() ? ds.sigma().get() : 10.0),
    _safe_sigma_sqr(fabs(_sigma) > 1e-5 ? sqr(_sigma) : 1e-5 )
  {}
    //    at(_centrex, _centrey) = -2.0 * total;



  Sharpen::Sharpen(Function2D::ptr func) :
    _width(1 + (2 * ceil(func->range()))), _height(_width),
    _centrex(ceil(func->range())), _centrey(_centrex),
    _values(NULL)
  {
    _values = (SAMPLE**)malloc(_height * sizeof(SAMPLE*));
    for (unsigned short int y = 0; y < _height; y++)
      _values[y] = (SAMPLE*)malloc(_width * sizeof(SAMPLE));

    SAMPLE total = 0;
#pragma omp parallel for schedule(dynamic, 1) shared(total)
    for (short unsigned int y = 0; y < _height; y++)
      for (short unsigned int x = 0; x < _width; x++)
	total += _at(x, y) = func->eval((double)x - _centrex, (double)y - _centrey);

    double old_centre = _at(_centrex, _centrey);
    _at(_centrex, _centrey) = -2.0 * total;
    total -= old_centre;
    total += _at(_centrex, _centrey);

    if (fabs(total) > 1e-5) {
      total = 1.0 / total;
#pragma omp parallel for schedule(dynamic, 1) shared(total)
      for (short unsigned int y = 0; y < _height; y++)
	for (short unsigned int x = 0; x < _width; x++)
	  _at(x,y) *= total;
    }
  }

  Sharpen::~Sharpen() {
    if (_values != NULL) {
      for (unsigned short int y = 0; y < _height; y++)
	free(_values[y]);
      free(_values);
      _values = NULL;
    }

    if (_row_counts != NULL) {
      free(_row_counts);
      _row_counts = NULL;
    }
    delete [] _rows;
  }

  void Sharpen::receive_image_header(ImageHeader::ptr header) {
    _row_counts = (unsigned int*)malloc(header->height() * sizeof(unsigned int));
    _rows = new ImageRow::ptr[header->height()];
    for (unsigned int y = 0; y < header->height(); y++) {
      _row_counts[y] = 0;
      _rows[y] = NULL;
    }

    _header = ImageHeader::ptr(new ImageHeader(header->width(), header->height()));

    if (header->profile() != NULL)
      _header->set_profile(header->profile());
    _header->set_cmsType(header->cmsType());
    if (header->xres().defined())
      _header->set_xres(header->xres());
    if (header->yres().defined())
      _header->set_yres(header->yres());

    this->_send_image_header(_header);
  }

  void Sharpen::_work_on_row(ImageRow::ptr row) {
    cmsUInt32Number cmsType = _sink_header->cmsType();

    if (T_FLOAT(cmsType) == 1) {
      switch (T_BYTES(cmsType)) {
      case 1: this->_sharpen_row<unsigned char>(row);
	break;

      case 2: this->_sharpen_row<unsigned short>(row);
	break;

      case 4: this->_sharpen_row<unsigned int>(row);
	break;

      default:
	break;
      }
    } else {
      switch (T_BYTES(cmsType)) {
      case 0: this->_sharpen_row<double>(row);
	break;

      case 4: this->_sharpen_row<float>(row);
	break;

      default:
	break;
      }
    }
  }

  template <typename P>
  void Sharpen::_sharpen_row(ImageRow::ptr row) {
    cmsUInt32Number cmsType = _header->cmsType();

    unsigned int y = row->y();
    short unsigned int ky_start = y < _centrey ? _centrey - y : 0;
    short unsigned int ky_end = y > _header->height() - _height + _centrey ? _header->height() + _centrey - y : _height;
    for (short unsigned int ky = ky_start; ky < ky_end; ky++) {
      unsigned int ny = y + ky - _centrey;

      if (!_rows[ny])
	_rows[ny] = ImageRow::ptr(_header->new_row(ny));

      P *outp = (P*)_rows[ny]->data();

      for (unsigned int x = 0; x < _header->width(); x++, outp += T_EXTRA(cmsType)) {
	short unsigned int kx_start = x < _centrex ? _centrex - x : 0;
	short unsigned int kx_end = x > _header->width() - _width + _centrex ? _header->width() + _centrex - x : _width;

	for (short unsigned int ky = ky_start; ky < ky_end; ky++) {
	  const SAMPLE *kp = &_at(kx_start, ky);
	  P *inp = (P*)row->data();
	  for (short unsigned int kx = kx_start; kx < kx_end; kx++, kp++, inp += T_EXTRA(cmsType)) {
	    _rows[ny]->lock();
	    for (unsigned char c = 0; c < T_CHANNELS(cmsType); c++, inp++, outp++)
	      *outp += (*inp) * *kp;
	    _rows[ny]->unlock();
	  }
	}
      }
      _row_counts[ny]++;
      if (_row_counts[ny] == _height) {
	_row_counts[ny] = 0;
	this->_send_image_row(_rows[ny]);
	_rows[ny].reset();
      }
    }
  }

}
