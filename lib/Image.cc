#include <stdio.h>
#include "Image.hh"

Image::Image(unsigned int w, unsigned int h) :
  _width(w),
  _height(h),
  rowdata(NULL)
{
  rowdata = (SAMPLE**)malloc(_height * sizeof(SAMPLE*));
  for (unsigned int y = 0; y < _height; y++)
    rowdata[y] = (SAMPLE*)malloc(_width * 3 * sizeof(SAMPLE));
}

Image::Image(Image& other) :
  _width(other._width),
  _height(other._height),
  rowdata(NULL)
{
  rowdata = (SAMPLE**)malloc(_height * sizeof(SAMPLE*));
  for (unsigned int y = 0; y < _height; y++) {
    rowdata[y] = (SAMPLE*)malloc(_width * 3 * sizeof(SAMPLE));
    memcpy(rowdata[y], other.rowdata[y], _width * 3 * sizeof(SAMPLE));
  }
}

Image::~Image() {
  if (rowdata != NULL) {
    for (unsigned int y = 0; y < _height; y++)
      free(rowdata[y]);
    free(rowdata);
    rowdata = NULL;
  }
  _width = _height = 0;
}

