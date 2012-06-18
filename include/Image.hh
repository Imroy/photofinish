#ifndef __IMAGE_HH__
#define __IMAGE_HH__

#include <stdlib.h>
#include <math.h>
#include <string.h>
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
    rowdata(NULL),
    _profile(p),
    _cmsType(t)
  {
    rowdata = (P**)malloc(_height * sizeof(P*));
    for (unsigned int y = 0; y < _height; y++)
      rowdata[y] = (P*)malloc(_width * _channels * sizeof(P));
  }

  Image(Image<P>& other) :
    _width(other._width),
    _height(other._height),
    _channels(other._channels),
    rowdata(NULL),
    _profile(other._profile),
    _cmsType(other._cmsType)
  {
    rowdata = (P**)malloc(_height * sizeof(P*));
    for (unsigned int y = 0; y < _height; y++) {
      rowdata[y] = (P*)malloc(_width * _channels * sizeof(P));
      memcpy(rowdata[y], other.rowdata[y], _width * _channels * sizeof(P));
    }
  }

  // In Image_png.hh
  Image(const char* filepath);

  ~Image() {
    for (unsigned int y = 0; y < _height; y++)
      free(rowdata[y]);
    free(rowdata);
    if (_profile)
      cmsCloseProfile(_profile);
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

  inline P* row(unsigned int y) {
    return &rowdata[y];
  }

  inline P* at(unsigned int x, unsigned int y) {
    return &rowdata[y][x * _channels];
  }

  inline P& at(unsigned int x, unsigned int y, unsigned char c) {
    return rowdata[y][c + (x * _channels)];
  }

  inline cmsHPROFILE profile(void) {
    return _profile;
  }

  inline cmsUInt32Number cmsType(void) {
    return _cmsType;
  }

  template <typename Q>
  inline void transform_from(Image<Q>& other, cmsUInt32Number intent, cmsUInt32Number flags) {
    if (_width != other._width) {
      fprintf(stderr, "transform_from: Image widths do not match.\n");
      exit(4);
    }
    if (_height != other._height) {
      fprintf(stderr, "transform_from: Image widths do not match.\n");
      exit(4);
    }
    cmsHTRANSFORM t = cmsCreateTransform(other._profile,
					 other._cmsType,
					 _profile,
					 _cmsType,
					 intent, flags);
    for (unsigned int y = 0; y < _height; y++)
      cmsDoTransform(t, other.rowdata[y], rowdata[y], _width);
  }
					 


};

#include "Image_png.hh"

#endif // __IMAGE_HH__
