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
#include <boost/algorithm/string.hpp>
#include "ImageFile.hh"

_ImageFile* ImageFile(const string filepath) {
  int len = filepath.length();
  if ((len > 4) && (boost::iequals(filepath.substr(len - 4, 4), ".png")))
    return (_ImageFile*)new PNGFile(filepath);
  if (((len > 5) && (boost::iequals(filepath.substr(len - 5, 5), ".jpeg")))
      || ((len > 4) && (boost::iequals(filepath.substr(len - 4, 4), ".jpg"))))
    return (_ImageFile*)new JPEGFile(filepath);

  return NULL;
}
