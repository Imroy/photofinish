/*
        Copyright 2014-2019 Ian Tester

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
#pragma once

#include <ostream>
#include <time.h>

namespace PhotoFinish {

  // Display some performance stats after some filters
  extern bool benchmark_mode;

  //! Class for doing nanosecond-accurate timings
  class Timer {
  private:
    bool _have_start, _have_end;
    timespec _start_time, _end_time;

  public:
    //! Empty constructor
    Timer();

    //! Record the start time
    inline void start(void) {
      if (clock_gettime(CLOCK_MONOTONIC, &_start_time) == 0)
	_have_start = true;
    }

    //! Record the end time
    inline void stop(void) {
      if (clock_gettime(CLOCK_MONOTONIC, &_end_time) == 0)
	_have_end = true;
    }

    //! Return the number of seconds elapsed
    double elapsed(void) const;

    //! Return the number of nanoseconds elapsed
    long long elapsed_ns(void) const;

  }; // class Benchmark

  std::ostream& operator<< (std::ostream& out, Timer t);

}; // namespace PhotoFinish;
