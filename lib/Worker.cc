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

  void Worker::_set_work_finished(void) {
    _work_finished = true;
    for (std::list<finished_hook>::iterator fhi = _finished_hooks.begin(); fhi != _finished_hooks.end(); fhi++)
      (*fhi)();
  }



  WorkGang::WorkGang() :
    _workers()
  {}

  void WorkGang::add_worker(Worker::ptr w) {
    _workers.push_back(w);
  }

  void WorkGang::work_until_finished(void) {
    if (_workers.size() == 0)
      return;

    Worker::list::iterator next = _workers.begin();
    omp_lock_t lock;
    omp_init_lock(&lock);

#pragma omp parallel shared(next)
    {
      unsigned int unfinished ;
      do {
	unfinished = 0;
	for (Worker::list::iterator wi = _workers.begin(); wi != _workers.end(); wi++)
	  if (!(*wi)->work_finished())
	    unfinished++;

	if (unfinished > 0) {
	  omp_set_lock(&lock);
	  unsigned int retries = _workers.size();
	  while (((*next)->work_finished()) && (retries > 0)) {
	    next++;
	    if (next == _workers.end())
	      next = _workers.begin();
	    retries--;
	  }
	  if (!(*next)->work_finished()) {
	    Worker::ptr worker = *next;
	    next++;
	    if (next == _workers.end())
	      next = _workers.begin();
	    omp_unset_lock(&lock);
	    worker->do_work();
	  } else
	    omp_unset_lock(&lock);
	}

      } while (unfinished > 0);
    }

    omp_destroy_lock(&lock);
  }

} // namespace PhotoFinish
