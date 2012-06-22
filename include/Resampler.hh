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
