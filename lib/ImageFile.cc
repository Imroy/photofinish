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
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "ImageFile.hh"
#include "Exception.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  ImageFile::ImageFile(const fs::path filepath) :
    _filepath(filepath)
  {}

  ImageFile::ptr ImageFile::create(const fs::path filepath) throw(UnknownFileType) {
    if (boost::iequals(filepath.extension().generic_string(), ".png"))
      return ImageFile::ptr(new PNGFile(filepath));

    if (boost::iequals(filepath.extension().generic_string(), ".jpeg")
	|| boost::iequals(filepath.extension().generic_string(), ".jpg"))
      return ImageFile::ptr(new JPEGFile(filepath));

    if (boost::iequals(filepath.extension().generic_string(), ".tiff")
	|| boost::iequals(filepath.extension().generic_string(), ".tif"))
      return ImageFile::ptr(new TIFFfile(filepath));

    throw UnknownFileType(filepath.generic_string());
  }

  ImageFile::ptr ImageFile::create(fs::path filepath, const std::string format) throw(UnknownFileType) {
    if (boost::iequals(format, "png"))
      return ImageFile::ptr(new PNGFile(filepath.replace_extension(".png")));

    if (boost::iequals(format, "jpeg")
	|| boost::iequals(format, "jpg"))
      return ImageFile::ptr(new JPEGFile(filepath.replace_extension(".jpeg")));

    if (boost::iequals(format, "tiff")
	|| boost::iequals(format, "tif"))
      return ImageFile::ptr(new TIFFfile(filepath.replace_extension(".tiff")));

    throw UnknownFileType(format);
  }

  Image::ptr ImageFile::read(void) const {
    Destination::ptr temp(new Destination);
    return this->read(temp);
  }


}
