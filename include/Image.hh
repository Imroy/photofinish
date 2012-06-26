#ifndef __IMAGE_HH__
#define __IMAGE_HH__

#include <stdlib.h>
#include <math.h>
#include <string.h>

// A floating-point, L*a*b* image class
class Image {
private:
  unsigned int _width, _height;
  SAMPLE **rowdata;

public:
  Image(unsigned int w, unsigned int h);
  Image(Image& other);
  ~Image();

  // Methods for accessing the private data
  inline unsigned int width(void) const {
    return _width;
  }

  inline unsigned int height(void) const {
    return _height;
  }

  inline SAMPLE* row(unsigned int y) const {
    return rowdata[y];
  }

  inline SAMPLE* at(unsigned int x, unsigned int y) const {
    return &rowdata[y][x * 3];
  }

  inline SAMPLE& at(unsigned int x, unsigned int y, unsigned char c) const {
    return rowdata[y][c + (x * 3)];
  }

private:

};

#endif // __IMAGE_HH__
