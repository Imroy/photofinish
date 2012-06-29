#include <stdlib.h>
#include "Image.hh"

namespace PhotoFinish {

  Image::Image(long int w, long int h) :
    _width(w),
    _height(h),
    rowdata(NULL)
  {
    rowdata = (SAMPLE**)malloc(_height * sizeof(SAMPLE*));
    for (long int y = 0; y < _height; y++)
      rowdata[y] = (SAMPLE*)malloc(_width * 3 * sizeof(SAMPLE));
  }

  Image::Image(const Image& other) :
    _width(other._width),
    _height(other._height),
    rowdata(NULL)
  {
    rowdata = (SAMPLE**)malloc(_height * sizeof(SAMPLE*));
    for (long int y = 0; y < _height; y++) {
      rowdata[y] = (SAMPLE*)malloc(_width * 3 * sizeof(SAMPLE));
      memcpy(rowdata[y], other.rowdata[y], _width * 3 * sizeof(SAMPLE));
    }
  }

  Image::~Image() {
    if (rowdata != NULL) {
      for (long int y = 0; y < _height; y++)
	free(rowdata[y]);
      free(rowdata);
      rowdata = NULL;
    }
    _width = _height = 0;
  }

}
