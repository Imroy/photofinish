#ifndef __RESAMPLER_HH__
#define __RESAMPLER_HH__

#include <stdlib.h>
#include <math.h>
#include "Filter.hh"

namespace PhotoFinish {

  // Currently only using Lanczos sampling
  class Resampler {
  private:
    long int *_N, **_Position;
    SAMPLE **_Weight;
    long int _to_size_i;

  public:
    Resampler(const _Filter* filter, double from_start, double from_size, long int from_max, double to_size);
    ~Resampler();

    // Methods for accessing the private data
    inline long int& N(long int i) { return _N[i]; }
    inline long int* Position(long int i) { return _Position[i]; }
    inline long int& Position(long int f, long int i) { return _Position[i][f]; }
    inline SAMPLE* Weight(long int i) { return _Weight[i]; }
    inline SAMPLE& Weight(long int f, long int i) { return _Weight[i][f]; }

  };

}

#endif // __RESAMPLER_HH__
