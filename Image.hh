#ifndef __IMAGE_HH__
#define __IMAGE_HH__

#include <stdlib.h>
#include <math.h>
#include <type_traits>
#include <lcms2.h>

inline double lanczos(double a, double x) {
  return (a * sin(x / a) * sin(M_PI * x / a)) / (M_PI * M_PI * x * x);
}

inline double lanczos(double a, double x, double y) {
  return lanczos(a, x) * lanczos(a, y);
}

template <typename P>
class Image {
private:
  unsigned int _width, _height;
  unsigned char _channels;
  P **rowdata;
  cmsHPROFILE _profile;
  cmsUInt32Number _cmsType;

public:
  Image(unsigned int w, unsigned int h, const cmsHPROFILE p, cmsUInt32Number t) :
    _width(w),
    _height(h),
    _channels(T_CHANNELS(t) + T_EXTRA(t)),
    _profile(p),
    _cmsType(t)
  {
    rowdata = (P**)malloc(_height * sizeof(P*));
    for (int y = 0; y < _height; y++)
      rowdata[y] = (P*)malloc(_width * _channels * sizeof(P));
  }

  // In Image_png.hh
  Image(const char* filepath);

  ~Image() {
    for (int y = 0; y < _height; y++)
      free(rowdata[y]);
    free(rowdata);
  }

  inline unsigned int width(void) {
    return _width;
  }

  inline unsigned int height(void) {
    return _height;
  }

  inline unsigned char channels(void) {
    return _channels;
  }

  inline P* at(unsigned int x, unsigned y) {
    return &rowdata[y][x * _channels];
  }

  inline P& at(unsigned int x, unsigned y, unsigned char c) {
    return rowdata[y][c + (x * _channels)];
  }

  inline cmsHPROFILE profile(void) {
    return _profile;
  }

  inline cmsUInt32Number cmsType(void) {
    return _cmsType;
  }

  

};

#include "Image_png.hh"

#endif // __IMAGE_HH__
