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
#include "Resampler.hh"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define sqr(x) ((x) * (x))

inline SAMPLE lanczos(double a, double ra, double x) {
  if (fabs(x) < 1e-6)
    return 1.0;
  double pix = M_PI * x;
  return (a * sin(pix) * sin(pix * ra)) / (sqr(M_PI) * sqr(x));
}

Resampler::Resampler(double a, unsigned int from_size, double to_size)
  : _to_size_i(ceil(to_size))
{
  double ra = 1.0 / a;
  double scale = to_size / from_size;
  _N = (unsigned int*)malloc(_to_size_i * sizeof(unsigned int));
  _Position = (unsigned int**)malloc(_to_size_i * sizeof(unsigned int*));
  _Weight = (SAMPLE**)malloc(_to_size_i * sizeof(SAMPLE*));

  double centre_offset = 0.5 / scale;

  double range = a;
  double norm_fact = 1.0;
  if (scale < 1.0) {
    range = a / scale;
    norm_fact = a / ceil(range);
  }

#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int i = 0; i < _to_size_i; i++) {
    double centre = i / scale + centre_offset;
    int left = floor(centre - range);
    if (left < 0)
      left = 0;
    unsigned int right = ceil(centre + range);
    if (right >= from_size)
      right = from_size - 1;
    _N[i] = right - left + 1;
    _Position[i] = (unsigned int*)malloc(_N[i] * sizeof(unsigned int));
    _Weight[i] = (SAMPLE*)malloc(_N[i] * sizeof(SAMPLE));
    unsigned int k = 0;
    for (unsigned int j = left; j <= right; j++, k++) {
      _Position[i][k] = j;
      _Weight[i][k] = lanczos(a, ra, (centre - j) * norm_fact);
    }
    // normalize the filter's weight's so the sum equals to 1.0, very important for avoiding box type of artifacts
    unsigned int max = _N[i];
    SAMPLE tot = 0.0;
    for (unsigned int k = 0; k < max; k++)
      tot += _Weight[i][k];
    if (tot != 0) // 0 should never happen except bug in filter
      for (unsigned int k = 0; k < max; k++)
	_Weight[i][k] /= tot;
  }

}

Resampler::~Resampler() {
  if (_N != NULL) {
    free(_N);
    _N = NULL;
  }

  if (_Position != NULL) {
    for (unsigned int i = 0; i < _to_size_i; i++)
      free(_Position[i]);
    free(_Position);
    _Position = NULL;
  }

  if (_Weight != NULL) {
    for (unsigned int i = 0; i < _to_size_i; i++)
      free(_Weight[i]);
    free(_Weight);
    _Weight = NULL;
  }
}
