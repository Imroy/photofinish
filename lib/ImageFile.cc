#include <string.h>
#include "ImageFile.hh"

_ImageFile::_ImageFile(const char* filepath) {
  int len = strlen(filepath);
  _filepath = (const char *)malloc(len * sizeof(char));
  strcpy((char*)_filepath, filepath);
}

_ImageFile::~_ImageFile() {
  if (_filepath != NULL) {
    free((void*)_filepath);
    _filepath = NULL;
  }
}

_ImageFile* ImageFile(const char* filepath) {
  int len = strlen(filepath);
  if ((len > 4) && (strcasecmp(filepath + len - 4, ".png") == 0))
    return (_ImageFile*)new PNGFile(filepath);
  if (((len > 5) && (strcasecmp(filepath + len - 5, ".jpeg") == 0))
      || ((len > 4) && (strcasecmp(filepath + len - 4, ".jpg") == 0)))
    return (_ImageFile*)new JPEGFile(filepath);

  return NULL;
}
