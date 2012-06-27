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
#include <math.h>
#include "Filter.hh"

SAMPLE Lanczos::eval(double x) {
  if (fabs(x) < 1e-6)
    return 1.0;
  double pix = M_PI * x;
  return (_radius * sin(pix) * sin(pix * _r_radius)) / (sqr(M_PI) * sqr(x));
}
