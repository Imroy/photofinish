#ifndef __IMAGE_HH__
#define __IMAGE_HH__

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <type_traits>

inline double lanczos(double a, double x) {
  return (a * sin(x / a) * sin(M_PI * x / a)) / (M_PI * M_PI * x * x);
}

inline double lanczos(double a, double x, double y) {
  return lanczos(a, x) * lanczos(a, y);
}

// A floating-point, L*a*b* image class
class Image {
private:
  unsigned int _width, _height;
  double **rowdata;

public:
  Image(unsigned int w, unsigned int h);
  Image(Image& other);
  Image(const char* filepath);	// Load a PNG image
  ~Image();

  inline unsigned int width(void) {
    return _width;
  }

  inline unsigned int height(void) {
    return _height;
  }

  inline double* row(unsigned int y) {
    return rowdata[y];
  }

  inline double* at(unsigned int x, unsigned int y) {
    return &rowdata[y][x * 3];
  }

  inline double& at(unsigned int x, unsigned int y, unsigned char c) {
    return rowdata[y][c + (x * 3)];
  }

};

#endif // __IMAGE_HH__
