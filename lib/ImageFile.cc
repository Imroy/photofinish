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
#include <boost/filesystem.hpp>
#include "ImageFile.hh"
#include "Exception.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  ImageFile::ImageFile() :
    _imagefile(NULL)
  {}

  ImageFile::ImageFile(const fs::path filepath) throw(UnknownFileType) :
    _imagefile(NULL)
  {
    if (boost::iequals(filepath.extension().generic_string(), ".png")) {
      _imagefile = new PNGFile(filepath);
      return;
    }

    if (boost::iequals(filepath.extension().generic_string(), ".jpeg")
	|| boost::iequals(filepath.extension().generic_string(), ".jpg")) {
      _imagefile = new JPEGFile(filepath);
      return;
    }

    throw UnknownFileType(filepath.generic_string());
  }

  ImageFile::ImageFile(fs::path filepath, const std::string format) throw(UnknownFileType) :
    _imagefile(NULL)
  {
    if (boost::iequals(format, "png")) {
      _imagefile = new PNGFile(filepath.replace_extension(".png"));
      return;
    }

    if (boost::iequals(format, "jpeg")
	|| boost::iequals(format, "jpg")) {
      _imagefile = new JPEGFile(filepath.replace_extension(".jpeg"));
      return;
    }

    throw UnknownFileType(format);
  }

  ImageFile::ImageFile(const ImageFile& other) :
    _imagefile(other._imagefile)
  {}

  ImageFile::~ImageFile() {
    if (_imagefile != NULL) {
      delete _imagefile;
      _imagefile = NULL;
    }
  }

  Image::ptr ImageFile::read(void) {
    if (_imagefile == NULL)
      throw Uninitialised("ImageFile");
    return _imagefile->read();
  }

  void ImageFile::write(Image::ptr img, const Destination &d) {
    if (_imagefile == NULL)
      throw Uninitialised("ImageFile");
    _imagefile->write(img, d);
  }

}
