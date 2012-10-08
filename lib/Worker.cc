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
#include <omp.h>
#include "Worker.hh"

namespace PhotoFinish {

  Worker::Worker() :
    _work_finished(false)
  {}



  WorkGang::WorkGang() {}

  void WorkGang::add_worker(Worker* w) {
    _workers.push_back(Worker::ptr(w));
  }

  void WorkGang::work_until_finished(void) {
    unsigned int next = 0;
    omp_lock_t lock;
    omp_init_lock(&lock);

#pragma omp parallel shared(next)
    {
      do {
	omp_set_lock(&lock);
	unsigned int i = next++;
	if (next > _workers.size())
	  next = 0;
	omp_unset_lock(&lock);

	if (i < _workers.size()) {
	  Worker::ptr worker = _workers[i];
	  worker->do_work();
	}
      } while (1);
    }

    omp_destroy_lock(&lock);
  }

} // namespace PhotoFinish
