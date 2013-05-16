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
    _filepath(filepath),
    _is_open(false)
  {}

  void ImageFile::extract_tags(Image::ptr img) {
    if (_is_open)
      throw FileOpenError("already open");

    Exiv2::Image::AutoPtr imagefile = Exiv2::ImageFactory::open(_filepath.native());
    assert(imagefile.get() != 0);

    for (auto ei : imagefile->exifData())
      img->EXIFtags().add(ei);

    for (auto ii : imagefile->iptcData())
      img->IPTCtags().add(ii);

    for (auto xi : imagefile->xmpData())
      img->XMPtags().add(xi);
  }

  void ImageFile::embed_tags(Image::ptr img) const {
    if (_is_open)
      throw FileOpenError("already open");

    Exiv2::Image::AutoPtr imagefile = Exiv2::ImageFactory::open(_filepath.native());
    assert(imagefile.get() != 0);

    imagefile->setExifData(img->EXIFtags());
    imagefile->setIptcData(img->IPTCtags());
    imagefile->setXmpData(img->XMPtags());
    imagefile->writeMetadata();
  }

  ImageFile::ptr ImageFile::create(const fs::path filepath) throw(UnknownFileType) {
    std::string ext = filepath.extension().generic_string().substr(1);
#ifdef HAZ_PNG
    if (boost::iequals(ext, "png"))
      return std::make_shared<PNGfile>(filepath);
#endif

#ifdef HAZ_JPEG
    if (boost::iequals(ext, "jpeg") || boost::iequals(ext, "jpg"))
      return std::make_shared<JPEGfile>(filepath);
#endif

#ifdef HAZ_TIFF
    if (boost::iequals(ext, "tiff") || boost::iequals(ext, "tif"))
      return std::make_shared<TIFFfile>(filepath);
#endif

#ifdef HAZ_JP2
    if (boost::iequals(ext, "jp2"))
      return std::make_shared<JP2file>(filepath);
#endif

#ifdef HAZ_WEBP
    if (boost::iequals(ext, "webp"))
      return std::make_shared<WebPfile>(filepath);
#endif

    throw UnknownFileType(filepath.generic_string());
  }

  ImageFile::ptr ImageFile::create(fs::path filepath, const std::string format) throw(UnknownFileType) {
#ifdef HAZ_PNG
    if (boost::iequals(format, "png"))
      return std::make_shared<PNGfile>(filepath.replace_extension(".png"));
#endif

#ifdef HAZ_JPEG
    if (boost::iequals(format, "jpeg")
	|| boost::iequals(format, "jpg"))
      return std::make_shared<JPEGfile>(filepath.replace_extension(".jpeg"));
#endif

#ifdef HAZ_TIFF
    if (boost::iequals(format, "tiff")
	|| boost::iequals(format, "tif"))
      return std::make_shared<TIFFfile>(filepath.replace_extension(".tiff"));
#endif

#ifdef HAZ_JP2
    if (boost::iequals(format, "jp2"))
      return std::make_shared<JP2file>(filepath.replace_extension(".jp2"));
#endif

#ifdef HAZ_WEBP
    if (boost::iequals(format, "webp"))
      return std::make_shared<WebPfile>(filepath.replace_extension(".webp"));
#endif

    if (boost::iequals(format, "sol"))
      return std::make_shared<SOLfile>(filepath.replace_extension(".bin"));

    throw UnknownFileType(format);
  }

  void ImageFile::add_variables(Destination::ptr dest, multihash& vars) {
    std::string format = dest->format().get();
    if (boost::iequals(format, "jpeg")
	|| boost::iequals(format, "jpg"))
      const_cast<D_JPEG&>(dest->jpeg()).add_variables(vars);

    if (boost::iequals(format, "tiff")
	|| boost::iequals(format, "tif"))
      const_cast<D_TIFF&>(dest->tiff()).add_variables(vars);

    if (boost::iequals(format, "jp2"))
      const_cast<D_JP2&>(dest->jp2()).add_variables(vars);

    if (boost::iequals(format, "webp"))
      const_cast<D_WebP&>(dest->webp()).add_variables(vars);
  }

  Image::ptr ImageFile::read(void) {
    auto temp = std::make_shared<Destination>();
    return this->read(temp);
  }

}
