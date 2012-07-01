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
      _imagefile = ptr(new PNGFile(filepath));
      return;
    }

    if (boost::iequals(filepath.extension().generic_string(), ".jpeg")
	|| boost::iequals(filepath.extension().generic_string(), ".jpg")) {
      _imagefile = ptr(new JPEGFile(filepath));
      return;
    }

    throw UnknownFileType(filepath.generic_string());
  }

  ImageFile::ImageFile(fs::path filepath, const std::string format) throw(UnknownFileType) :
    _imagefile(NULL)
  {
    if (boost::iequals(format, "png")) {
      _imagefile = ptr(new PNGFile(filepath.replace_extension(".png")));
      return;
    }

    if (boost::iequals(format, "jpeg")
	|| boost::iequals(format, "jpg")) {
      _imagefile = ptr(new JPEGFile(filepath.replace_extension(".jpeg")));
      return;
    }

    throw UnknownFileType(format);
  }

  Image::ptr ImageFile::read(void) const {
    if (_imagefile == NULL)
      throw Uninitialised("ImageFile");
    return _imagefile->read();
  }

  void ImageFile::write(Image::ptr img, const Destination &d) const {
    if (_imagefile == NULL)
      throw Uninitialised("ImageFile");
    _imagefile->write(img, d);
  }

}
