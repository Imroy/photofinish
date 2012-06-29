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

    throw UnknownFileType(filepath);
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

  const Image& ImageFile::read(void) {
    if (_imagefile == NULL)
      throw Uninitialised("ImageFile");
    return _imagefile->read();
  }

  void ImageFile::write(const Image& img, const Destination &d) {
    if (_imagefile == NULL)
      throw Uninitialised("ImageFile");
    _imagefile->write(img, d);
  }

}
