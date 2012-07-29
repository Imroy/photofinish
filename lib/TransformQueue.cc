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
#include "TransformQueue.hh"

namespace PhotoFinish {

  transform_queue::transform_queue(Image::ptr img, int channels, cmsHTRANSFORM transform) :
    _rowpointers((short unsigned int**)malloc(img->height() * sizeof(short unsigned int*))),
    _rowlocks(img->height()),
    _rowlen(img->width() * channels * sizeof(short unsigned int)),
    _queue_lock((omp_lock_t*)malloc(sizeof(omp_lock_t))),
    _img(img),
    _transform(transform),
    _finished(false)
  {
    omp_init_lock(_queue_lock);

    for (unsigned int y = 0; y < _img->height(); y++) {
      _rowpointers[y] = NULL;
      _rowlocks[y] = (omp_lock_t*)malloc(sizeof(omp_lock_t));
      omp_init_lock(_rowlocks[y]);
    }
  }

  transform_queue::~transform_queue() {
    for (unsigned int y = 0; y < _img->height(); y++) {
      omp_destroy_lock(_rowlocks[y]);
      free(_rowlocks[y]);
    }
    free(_rowpointers);
    omp_destroy_lock(_queue_lock);
    free(_queue_lock);
  }

  bool transform_queue::empty(void) const {
    omp_set_lock(_queue_lock);
    bool ret = _rowqueue.empty();
    omp_unset_lock(_queue_lock);
    return ret;
  }

  short unsigned int* transform_queue::row(unsigned int y) const {
    omp_set_lock(_rowlocks[y]);
    short unsigned int *ret = _rowpointers[y];
    omp_unset_lock(_rowlocks[y]);
    return ret;
  }

  void transform_queue::add(unsigned int num, void* data) {
    omp_set_lock(_queue_lock);
    _rowqueue.push(new row_t(num, data));
    omp_unset_lock(_queue_lock);
  }

  void transform_queue::reader_process_row(void) {
    omp_set_lock(_queue_lock);
    if (!_rowqueue.empty()) {
      row_t *row = _rowqueue.front();
      _rowqueue.pop();

      omp_set_lock(_rowlocks[row->first]);	// Take out row lock before relinquishing the queue lock
      omp_unset_lock(_queue_lock);

      cmsDoTransform(_transform, row->second, _img->row(row->first), _img->width());
      free(row->second);

      omp_unset_lock(_rowlocks[row->first]);
      delete row;
    } else
      omp_unset_lock(_queue_lock);
  }

  void transform_queue::writer_process_row(void) {
    omp_set_lock(_queue_lock);
    if (!_rowqueue.empty()) {
      row_t *row = _rowqueue.front();
      _rowqueue.pop();

      omp_set_lock(_rowlocks[row->first]);	// Take out row lock before relinquishing the queue lock
      omp_unset_lock(_queue_lock);

      if (row->second == NULL)
	row->second = malloc(_rowlen);

      cmsDoTransform(_transform, _img->row(row->first), row->second, _img->width());

      _rowpointers[row->first] = (short unsigned int*)row->second;
      omp_unset_lock(_rowlocks[row->first]);
      delete row;
    } else
      omp_unset_lock(_queue_lock);
  }

}
