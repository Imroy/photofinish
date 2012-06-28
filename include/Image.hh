#ifndef __IMAGE_HH__
#define __IMAGE_HH__

#include <stdlib.h>
#include <math.h>
#include <string.h>

namespace PhotoFinish {

  // A floating-point, L*a*b* image class
  class Image {
  private:
    long int _width, _height;
    SAMPLE **rowdata;

  public:
    Image(long int w, long int h);
    Image(const Image& other);
    ~Image();

    // Methods for accessing the private data
    inline long int width(void) const { return _width; }
    inline long int height(void) const { return _height; }
    inline SAMPLE* row(long int y) const { return rowdata[y]; }
    inline SAMPLE* at(long int x, long int y) const { return &rowdata[y][x * 3]; }
    inline SAMPLE& at(long int x, long int y, unsigned char c) const { return rowdata[y][c + (x * 3)]; }

  private:

  };

}

#endif // __IMAGE_HH__
