/*
	Copyright 2014 Ian Tester

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
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include "ImageFile.hh"
#include "Exception.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  ImageFilepath::ImageFilepath(const fs::path filepath, const std::string format) :
    _filepath(filepath),
    _format(format)
  {}

  ImageFilepath::ImageFilepath(const fs::path filepath) :
    _filepath(filepath)
  {
    std::string ext = filepath.extension().generic_string().substr(1);
    bool unknown = true;
#ifdef HAZ_PNG
    if (boost::iequals(ext, "png")) {
      _format = "png";
      unknown = false;
    }
#endif

#ifdef HAZ_JPEG
    if (boost::iequals(ext, "jpeg") || boost::iequals(ext, "jpg")) {
      _format = "jpeg";
      unknown = false;
    }
#endif

#ifdef HAZ_TIFF
    if (boost::iequals(ext, "tiff") || boost::iequals(ext, "tif")) {
      _format = "tiff";
      unknown = false;
    }
#endif

#ifdef HAZ_JP2
    if (boost::iequals(ext, "jp2")) {
      _format = "jp2";
      unknown = false;
    }
#endif

#ifdef HAZ_WEBP
    if (boost::iequals(ext, "webp")) {
      _format = "webp";
      unknown = false;
    }
#endif

#ifdef HAZ_JXR
    if (boost::iequals(ext, "jxr") || boost::iequals(ext, "wdp") || boost::iequals(ext, "hdp")) {
      _format = "jxr";
      unknown = false;
    }
#endif

#ifdef HAZ_FLIF
    if (boost::iequals(ext, "flif")) {
      _format = "flif";
      unknown = false;
    }
#endif

    if (unknown)
      throw UnknownFileType(filepath.generic_string());
  }

  fs::path ImageFilepath::fixed_filepath(void) const {
    fs::path fp(_filepath);
#ifdef HAZ_PNG
    if (boost::iequals(_format, "png"))
      return fp.replace_extension(".png");
#endif

#ifdef HAZ_JPEG
    if (boost::iequals(_format, "jpeg")
        || boost::iequals(_format, "jpg"))
      return fp.replace_extension(".jpeg");
#endif

#ifdef HAZ_TIFF
    if (boost::iequals(_format, "tiff")
        || boost::iequals(_format, "tif"))
      return fp.replace_extension(".tiff");
#endif

#ifdef HAZ_JP2
    if (boost::iequals(_format, "jp2"))
      return fp.replace_extension(".jp2");
#endif

#ifdef HAZ_WEBP
    if (boost::iequals(_format, "webp"))
      return fp.replace_extension(".webp");
#endif

#ifdef HAZ_JXR
    if (boost::iequals(_format, "jxr"))
      return fp.replace_extension(".jxr");
#endif

#ifdef HAZ_FLIF
    if (boost::iequals(_format, "flif"))
      return fp.replace_extension(".flif");
#endif

    if (boost::iequals(_format, "sol"))
      return fp.replace_extension(".bin");

    throw UnknownFileType(_format);
  }



  ImageReader::ImageReader(const fs::path fp) :
    _filepath(fp),
    _is_open(false)
  {}

  void ImageReader::extract_tags(Image::ptr img) {
    if (_is_open)
      throw FileOpenError("already open");

    Exiv2::Image::AutoPtr imagefile = Exiv2::ImageFactory::open(_filepath.native());
    assert(imagefile.get() != 0);
    imagefile->readMetadata();

    for (auto ei : imagefile->exifData())
      img->EXIFtags().add(ei);

    for (auto ii : imagefile->iptcData())
      img->IPTCtags().add(ii);

    for (auto xi : imagefile->xmpData())
      img->XMPtags().add(xi);
  }

  ImageReader::ptr ImageReader::open(const ImageFilepath& ifp) {
    ImageReader *ir = nullptr;
#ifdef HAZ_PNG
    if (boost::iequals(ifp.format(), "png"))
      ir = new PNGreader(ifp.filepath());
#endif

#ifdef HAZ_JPEG
    if (boost::iequals(ifp.format(), "jpeg"))
      ir = new JPEGreader(ifp.filepath());
#endif

#ifdef HAZ_TIFF
    if (boost::iequals(ifp.format(), "tiff"))
      ir = new TIFFreader(ifp.filepath());
#endif

#ifdef HAZ_JP2
    if (boost::iequals(ifp.format(), "jp2"))
      ir = new JP2reader(ifp.filepath());
#endif

#ifdef HAZ_WEBP
    if (boost::iequals(ifp.format(), "webp"))
      ir = new WebPreader(ifp.filepath());
#endif

#ifdef HAZ_JXR
    if (boost::iequals(ifp.format(), "jxr"))
      ir = new JXRreader(ifp.filepath());
#endif

#ifdef HAZ_FLIF
    if (boost::iequals(ifp.format(), "flif"))
      ir = new FLIFreader(ifp.filepath());
#endif

    if (ir == nullptr)
      throw UnknownFileType(ifp.format());

    return ImageReader::ptr(ir);
  }

  Image::ptr ImageReader::read(void) {
    auto temp = std::make_shared<Destination>();
    return this->read(temp);
  }


  ImageWriter::ImageWriter(const fs::path fp) :
    _filepath(fp),
    _is_open(false)
  {}

  void ImageWriter::embed_tags(Image::ptr img) const {
    if (_is_open)
      throw FileOpenError("already open");

    Exiv2::Image::AutoPtr imagefile = Exiv2::ImageFactory::open(_filepath.native());
    assert(imagefile.get() != 0);

    imagefile->setExifData(img->EXIFtags());
    imagefile->setIptcData(img->IPTCtags());
    imagefile->setXmpData(img->XMPtags());
    imagefile->writeMetadata();
  }

  ImageWriter::ptr ImageWriter::open(const ImageFilepath& ifp) {
    ImageWriter *iw = nullptr;
#ifdef HAZ_PNG
    if (boost::iequals(ifp.format(), "png"))
      iw = new PNGwriter(ifp.fixed_filepath());
#endif

#ifdef HAZ_JPEG
    if (boost::iequals(ifp.format(), "jpeg")
        || boost::iequals(ifp.format(), "jpg"))
      iw = new JPEGwriter(ifp.fixed_filepath());
#endif

#ifdef HAZ_TIFF
    if (boost::iequals(ifp.format(), "tiff")
        || boost::iequals(ifp.format(), "tif"))
      iw = new TIFFwriter(ifp.fixed_filepath());
#endif

#ifdef HAZ_JP2
    if (boost::iequals(ifp.format(), "jp2"))
      iw = new JP2writer(ifp.fixed_filepath());
#endif

#ifdef HAZ_WEBP
    if (boost::iequals(ifp.format(), "webp"))
      iw = new WebPwriter(ifp.fixed_filepath());
#endif

#ifdef HAZ_JXR
    if (boost::iequals(ifp.format(), "jxr"))
      iw = new JXRwriter(ifp.fixed_filepath());
#endif

#ifdef HAZ_FLIF
    if (boost::iequals(ifp.format(), "flif"))
      iw = new FLIFwriter(ifp.fixed_filepath());
#endif

    if (boost::iequals(ifp.format(), "sol"))
      iw = new SOLwriter(ifp.fixed_filepath());

    if (iw == nullptr)
      throw UnknownFileType(ifp.format());

    return ImageWriter::ptr(iw);
  }

  void ImageWriter::add_variables(Destination::ptr dest, multihash& vars) {
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

    if (boost::iequals(format, "jxr"))
      const_cast<D_JXR&>(dest->jxr()).add_variables(vars);

    if (boost::iequals(format, "flif"))
      const_cast<D_FLIF&>(dest->flif()).add_variables(vars);
  }

}
