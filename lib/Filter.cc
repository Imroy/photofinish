#include <math.h>
#include "Filter.hh"

SAMPLE Lanczos::eval(double x) {
  if (fabs(x) < 1e-6)
    return 1.0;
  double pix = M_PI * x;
  return (_radius * sin(pix) * sin(pix * _r_radius)) / (sqr(M_PI) * sqr(x));
}
