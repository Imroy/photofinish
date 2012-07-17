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
#include "TransformQueue.hh"

namespace PhotoFinish {

  transform_queue::transform_queue(Image::ptr img, int channels, cmsHTRANSFORM transform, unsigned char **destrows) :
    _rows((short unsigned int**)malloc(img->height() * sizeof(short unsigned int*))),
    _rowlocks(img->height()),
    _destrows(destrows),
    _rowlen(img->width() * channels * sizeof(short unsigned int)),
    _queue_lock((omp_lock_t*)malloc(sizeof(omp_lock_t))),
    _img(img),
    _transform(transform)
  {
    omp_init_lock(_queue_lock);

    for (long int y = 0; y < _img->height(); y++) {
      _rownumbers.push(y);
      _rows[y] = NULL;
      _rowlocks[y] = (omp_lock_t*)malloc(sizeof(omp_lock_t));
      omp_init_lock(_rowlocks[y]);
    }
  }

  transform_queue::~transform_queue() {
    for (long int y = 0; y < _img->height(); y++) {
      omp_destroy_lock(_rowlocks[y]);
      free(_rowlocks[y]);
    }
    free(_rows);
    omp_destroy_lock(_queue_lock);
    free(_queue_lock);
  }

  bool transform_queue::empty(void) const {
    omp_set_lock(_queue_lock);
    bool ret = _rownumbers.empty();
    omp_unset_lock(_queue_lock);
    return ret;
  }

  short unsigned int* transform_queue::row(long int y) const {
    omp_set_lock(_rowlocks[y]);
    short unsigned int *ret = _rows[y];
    omp_unset_lock(_rowlocks[y]);
    return ret;
  }

  //! Pull a row off of the workqueue and transform it using LCMS
  void transform_queue::process_row(void) {
    omp_set_lock(_queue_lock);
    if (!_rownumbers.empty()) {
      long int y = _rownumbers.front();
      _rownumbers.pop();

      omp_set_lock(_rowlocks[y]);	// Take out row lock before relinquishing the queue lock
      omp_unset_lock(_queue_lock);

      short unsigned int *destrow;
      if (_destrows == NULL)
	destrow = (short unsigned int*)malloc(_rowlen);
      else
	destrow = (short unsigned int*)_destrows[y];

      cmsDoTransform(_transform, _img->row(y), destrow, _img->width());

      _rows[y] = destrow;
      omp_unset_lock(_rowlocks[y]);
    } else
      omp_unset_lock(_queue_lock);
  }

}
