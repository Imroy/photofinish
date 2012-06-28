#include "Resampler.hh"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

namespace PhotoFinish {

  Resampler::Resampler(const _Filter* filter, double from_start, double from_size, long int from_max, double to_size)
    : _to_size_i(ceil(to_size))
  {
    double scale = from_size / to_size;
    _N = (long int*)malloc(_to_size_i * sizeof(long int));
    _Position = (long int**)malloc(_to_size_i * sizeof(long int*));
    _Weight = (SAMPLE**)malloc(_to_size_i * sizeof(SAMPLE*));

    double range = filter->radius();
    double norm_fact = 1.0;
    if (scale >= 1.0) {
      range = filter->radius() * scale;
      norm_fact = filter->radius() / ceil(range);
    }

#pragma omp parallel for schedule(dynamic, 1)
    for (long int i = 0; i < _to_size_i; i++) {
      double centre = from_start + (i * scale);
      int left = floor(centre - range);
      if (left < 0)
	left = 0;
      long int right = ceil(centre + range);
      if (right >= from_max)
	right = from_max - 1;
      _N[i] = right - left + 1;
      _Position[i] = (long int*)malloc(_N[i] * sizeof(long int));
      _Weight[i] = (SAMPLE*)malloc(_N[i] * sizeof(SAMPLE));
      long int k = 0;
      for (long int j = left; j <= right; j++, k++) {
	_Position[i][k] = j;
	_Weight[i][k] = filter->eval((centre - j) * norm_fact);
      }
      // normalize the filter's weight's so the sum equals to 1.0, very important for avoiding box type of artifacts
      long int max = _N[i];
      SAMPLE tot = 0.0;
      for (long int k = 0; k < max; k++)
	tot += _Weight[i][k];
      if (tot != 0) // 0 should never happen except bug in filter
	for (long int k = 0; k < max; k++)
	  _Weight[i][k] /= tot;
    }

  }

  Resampler::~Resampler() {
    if (_N != NULL) {
      free(_N);
      _N = NULL;
    }

    if (_Position != NULL) {
      for (long int i = 0; i < _to_size_i; i++)
	free(_Position[i]);
      free(_Position);
      _Position = NULL;
    }

    if (_Weight != NULL) {
      for (long int i = 0; i < _to_size_i; i++)
	free(_Weight[i]);
      free(_Weight);
      _Weight = NULL;
    }
  }

}
