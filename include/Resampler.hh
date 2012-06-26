#ifndef __RESAMPLER_HH__
#define __RESAMPLER_HH__

#include <stdlib.h>
#include <math.h>

// Currently only using Lanczos sampling
class Resampler {
private:
  unsigned int *_N, **_Position;
  SAMPLE **_Weight;
  unsigned int _to_size_i;

public:
  Resampler(double a, double from_start, double from_size, unsigned int from_max, double to_size);
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

  inline SAMPLE* Weight(unsigned int i) {
    return _Weight[i];
  }

  inline SAMPLE& Weight(unsigned int f, unsigned int i) {
    return _Weight[i][f];
  }

};

#endif // __RESAMPLER_HH__
