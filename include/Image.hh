#ifndef __IMAGE_HH__
#define __IMAGE_HH__

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <type_traits>
#include <lcms2.h>

// A floating-point, L*a*b* image class
class Image {
private:
  unsigned int _width, _height;
  double **rowdata;

public:
  Image(unsigned int w, unsigned int h);
  Image(Image& other);
  ~Image();

  // Methods for accessing the private data
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

  // Resize the image using a Lanczos filter
  Image& resize(double nw, double nh, double a);

private:
  // Private methods for 1-dimensional scaling
  Image* _resize_w(double nw, double a);
  Image* _resize_h(double nh, double a);

};

#endif // __IMAGE_HH__
