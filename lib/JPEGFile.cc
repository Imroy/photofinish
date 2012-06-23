#include <omp.h>
#include "ImageFile.hh"

JPEGFile::JPEGFile(const char* filepath) :
  _ImageFile(filepath)
{}

bool JPEGFile::set_bit_depth(int bit_depth) {
  if (bit_depth == 8)
    return true;
  return false;
}

bool JPEGFile::set_profile(cmsHPROFILE profile, cmsUInt32Number intent) {
  return true;
}

Image* JPEGFile::read(void) {
  return NULL;
}

bool JPEGFile::write(Image* img) {
  return false;
}

