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
#include "Benchmark.hh"

namespace PhotoFinish {

  bool benchmark_mode = false;

  Timer::Timer() :
    _have_start(false), _have_end(false)
  {}

  double Timer::elapsed(void) const {
    if (!_have_start || !_have_end)
      return -1;

    return _end_time.tv_sec - _start_time.tv_sec + ((_end_time.tv_nsec - _start_time.tv_nsec) * 1e-9);
  }

  long long Timer::elapsed_ns(void) const {
    if (!_have_start || !_have_end)
      return -1;

    return ((_end_time.tv_sec - _start_time.tv_sec) * 1e+9) + _end_time.tv_nsec - _start_time.tv_nsec;
  }

  std::ostream& operator<< (std::ostream& out, Timer t) {
    long long divisor = 1;
    std::string prefix = "n";
    long long ns = t.elapsed_ns();

    if (ns >= 1e+9) {
      divisor = 1e+9;
      prefix = "";
    } else if (ns >= 1e+6) {
      divisor = 1e+6;
      prefix = "m";
    } else if (ns >= 1e+3) {
      divisor = 1e+3;
      prefix = "μ";
    }

    long long whole = ns / divisor;
    long long part = (ns % divisor) * 100 / divisor;

    out << whole << "." << part << " " << prefix << "s";
    return out;
  }


}; // namespace PhotoFinish

