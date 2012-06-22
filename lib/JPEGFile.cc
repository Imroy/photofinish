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

Image& JPEGFile::read(void) {
}

bool JPEGFile::write(Image& img) {
  return true;
}

