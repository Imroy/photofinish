/*
	Copyright 2012 Ian Tester

	This file is part of Photo Finish.

	Photo Finish is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Photo Finish is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Photo Finish.  If not, see <http://www.gnu.org/licenses/>.
*/
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
  if ((len > 3) && (strcasecmp(filepath + len - 3, ".png") == 0))
    return (_ImageFile*)new PNGFile(filepath);
  if (((len > 4) && (strcasecmp(filepath + len - 4, ".jpeg") == 0))
      || ((len > 3) && (strcasecmp(filepath + len - 3, ".jpg") == 0)))
    return (_ImageFile*)new JPEGFile(filepath);

  return NULL;
}
