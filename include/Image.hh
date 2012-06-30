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
    bool _greyscale;		// Used by readers and writers when converting colour spaces
    SAMPLE **rowdata;

  public:
    Image(long int w, long int h);
    Image(const Image& other);
    ~Image();

    // Methods for accessing the private data
    inline long int width(void) const { return _width; }
    inline long int height(void) const { return _height; }
    inline bool is_greyscale(void) const { return _greyscale; }
    inline bool is_colour(void) const { return !_greyscale; }
    inline void set_greyscale(bool g = true) { _greyscale = g; }
    inline void set_colour(bool c = true) { _greyscale = !c; }
    inline SAMPLE* row(long int y) const { return rowdata[y]; }
    inline SAMPLE* at(long int x, long int y) const { return &rowdata[y][x * 3]; }
    inline SAMPLE& at(long int x, long int y, unsigned char c) const { return rowdata[y][c + (x * 3)]; }

  private:

  };

}

#endif // __IMAGE_HH__
