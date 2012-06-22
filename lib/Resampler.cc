#include "Resampler.hh"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define sqr(x) ((x) * (x))

inline double lanczos(double a, double ra, double x) {
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
  _Weight = (double**)malloc(_to_size_i * sizeof(double*));

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
    _Weight[i] = (double*)malloc(_N[i] * sizeof(double));
    unsigned int k = 0;
    for (unsigned int j = left; j <= right; j++, k++) {
      _Position[i][k] = j;
      _Weight[i][k] = lanczos(a, ra, (centre - j) * norm_fact);
    }
    // normalize the filter's weight's so the sum equals to 1.0, very important for avoiding box type of artifacts
    unsigned int max = _N[i];
    double tot = 0.0;
    for (unsigned int k = 0; k < max; k++)
      tot += _Weight[i][k];
    if (tot != 0) // 0 should never happen except bug in filter
      for (unsigned int k = 0; k < max; k++)
	_Weight[i][k] /= tot;
  }

}

Resampler::~Resampler() {
#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int i = 0; i < _to_size_i; i++) {
    free(_Position[i]);
    free(_Weight[i]);
  }
  free(_N);
  free(_Position);
  free(_Weight);
}
