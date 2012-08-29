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
#include <stdlib.h>
#include <string.h>
#include "TransformQueue.hh"

namespace PhotoFinish {

  transform_queue::transform_queue(Destination::ptr dest) :
    _rows(),
    _rowlocks(),
    _rowlen(0),
    _queue_lock((omp_lock_t*)malloc(sizeof(omp_lock_t))),
    _dest(dest),
    _transform(NULL),
    _finished(false)
  {
    omp_init_lock(_queue_lock);
  }

#define T_BYTES_REAL(t) (T_BYTES(t) == 0 ? 8 : T_BYTES(t))

  transform_queue::transform_queue(Destination::ptr dest, Image::ptr img, cmsUInt32Number cmsType, cmsHTRANSFORM transform) :
    _rows(new row_ptr [img->height()]),
    _rowlocks(),
    _rowlen(img->width() * (T_CHANNELS(cmsType) + T_EXTRA(cmsType)) * T_BYTES_REAL(cmsType)),
    _queue_lock((omp_lock_t*)malloc(sizeof(omp_lock_t))),
    _dest(dest),
    _img(img),
    _transform(transform),
    _finished(false)
  {
    omp_init_lock(_queue_lock);

    _rowlocks.reserve(img->height());
    for (unsigned int y = 0; y < _img->height(); y++) {
      _rowlocks[y] = (omp_lock_t*)malloc(sizeof(omp_lock_t));
      omp_init_lock(_rowlocks[y]);
    }
  }

  transform_queue::~transform_queue() {
    for (unsigned int y = 0; y < _img->height(); y++) {
      omp_destroy_lock(_rowlocks[y]);
      free(_rowlocks[y]);
    }
    delete [] _rows;
    omp_destroy_lock(_queue_lock);
    free(_queue_lock);
  }

  void transform_queue::set_image(Image::ptr img, cmsUInt32Number cmsType) {
    _rows = new row_ptr [img->height()];
    _rowlocks.reserve(img->height());
    _rowlen = img->width() * (T_CHANNELS(cmsType) + T_EXTRA(cmsType)) * T_BYTES_REAL(cmsType);
    _img = img;

    for (unsigned int y = 0; y < _img->height(); y++) {
      _rowlocks[y] = (omp_lock_t*)malloc(sizeof(omp_lock_t));
      omp_init_lock(_rowlocks[y]);
    }
  }

  bool transform_queue::empty(void) const {
    omp_set_lock(_queue_lock);
    bool ret = _rowqueue.empty();
    omp_unset_lock(_queue_lock);
    return ret;
  }

  void* transform_queue::row(unsigned int y) const {
    omp_set_lock(_rowlocks[y]);
    void *ret = NULL;
    if (_rows[y])
      ret = _rows[y]->data;
    omp_unset_lock(_rowlocks[y]);
    return ret;
  }

  void transform_queue::free_row(unsigned int y) {
    omp_set_lock(_rowlocks[y]);
    if (_rows[y] && (_rows[y]->data != NULL)) {
      free(_rows[y]->data);
      _rows[y]->data = NULL;
    }
    omp_unset_lock(_rowlocks[y]);
  }

  void transform_queue::add(unsigned int num, void* data) {
    omp_set_lock(_queue_lock);
    _rowqueue.push(row_ptr(new row_t(num, data, false)));
    omp_unset_lock(_queue_lock);
  }

  void transform_queue::add_copy(unsigned int num, void* data) {
    omp_set_lock(_queue_lock);
    void *new_row = malloc(_rowlen);
    memcpy(new_row, data, _rowlen);

    _rowqueue.push(row_ptr(new row_t(num, new_row, true)));
    omp_unset_lock(_queue_lock);
  }

  void transform_queue::reader_process_row(void) {
    if (!_img || (_transform == NULL))
      return;

    omp_set_lock(_queue_lock);
    if (!_rowqueue.empty()) {
      row_ptr row = _rowqueue.front();
      _rowqueue.pop();

      omp_set_lock(_rowlocks[row->y]);	// Take out row lock before relinquishing the queue lock
      omp_unset_lock(_queue_lock);

      cmsDoTransform(_transform, row->data, _img->row(row->y), _img->width());
      if (row->our_data)
	free(row->data);

      omp_unset_lock(_rowlocks[row->y]);
    } else
      omp_unset_lock(_queue_lock);
  }

  void transform_queue::writer_process_row(void) {
    if (!_img || (_transform == NULL))
      return;

    omp_set_lock(_queue_lock);
    if (!_rowqueue.empty()) {
      row_ptr row = _rowqueue.front();
      _rowqueue.pop();

      omp_set_lock(_rowlocks[row->y]);	// Take out row lock before relinquishing the queue lock
      omp_unset_lock(_queue_lock);

      if (row->data == NULL)
	row->data = malloc(_rowlen);

      cmsDoTransform(_transform, _img->row(row->y), row->data, _img->width());

      _rows[row->y] = row;
      omp_unset_lock(_rowlocks[row->y]);
    } else
      omp_unset_lock(_queue_lock);
  }

}
