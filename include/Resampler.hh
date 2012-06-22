#ifndef __RESAMPLER_HH__
#define __RESAMPLER_HH__

#include <stdlib.h>
#include <math.h>

// Currently only using Lanczos sampling
class Resampler {
private:
  double _a, _ra;
  double _from_size, _to_size;
  unsigned int *_N, **_Position;
  double **_Weight;
  unsigned int _to_size_i, _factors;

public:
  Resampler(double a, double from, double to);
  ~Resampler();

  // Methods for accessing the private data
  inline double from_size(void) {
    return _from_size;
  }

  inline double to_size(void) {
    return _to_size;
  }

  inline unsigned int factors(void) {
    return _factors;
  }

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
