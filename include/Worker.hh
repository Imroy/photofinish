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
#ifndef __WORKER_HH__
#define __WORKER_HH__

#include <memory>
#include <list>
#include <omp.h>

namespace PhotoFinish {

  //! Abstract base "role" class for any classes that do "work"
  class Worker {
  protected:
    bool _work_finished;

    inline void _set_work_finished(void) { _work_finished = true; }

  public:
    //! Empty constructor
    Worker();

    inline bool work_finished(void) const { return _work_finished; }

    //! Perform a single unit of work e.g process a row or pixels
    virtual void do_work(void) = 0;

    typedef std::shared_ptr<Worker> ptr;
    typedef std::list<ptr> list;
  }; // class Worker



  //! 
  class WorkGang {
  private:
    Worker::list _workers;

  public:
    //! Empty constructor
    WorkGang();

    void add_worker(Worker::ptr w);

    void work_until_finished(void);

    typedef std::shared_ptr<WorkGang> ptr;
  }; // class WorkGang

} // namespace PhotoFinish

#endif // __WORKER_HH__
