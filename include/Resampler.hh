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
#ifndef __RESAMPLER_HH__
#define __RESAMPLER_HH__

#include <stdlib.h>
#include <math.h>

// Currently only using Lanczos sampling
class Resampler {
private:
  unsigned int *_N, **_Position;
  double **_Weight;
  unsigned int _to_size_i;

public:
  Resampler(double a, unsigned int from_size, double to_size);
  ~Resampler();

  // Methods for accessing the private data
  inline unsigned int& N(unsigned int i) {
    return _N[i];
  }

  inline unsigned int* Position(unsigned int i) {
    return _Position[i];
  }

  inline unsigned int& Position(unsigned int f, unsigned int i) {
    return _Position[i][f];
  }

  inline double* Weight(unsigned int i) {
    return _Weight[i];
  }

  inline double& Weight(unsigned int f, unsigned int i) {
    return _Weight[i][f];
  }

};

#endif // __RESAMPLER_HH__
