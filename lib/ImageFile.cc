#include <boost/algorithm/string.hpp>
#include "ImageFile.hh"
#include "Exception.hh"

namespace PhotoFinish {

  ImageFile::ImageFile() :
    _imagefile(NULL)
  {}

  ImageFile::ImageFile(const std::string filepath) throw(UnknownFileType) :
    _imagefile(NULL)
  {
    int len = filepath.length();
    if ((len > 4) && (boost::iequals(filepath.substr(len - 4, 4), ".png"))) {
      _imagefile = new PNGFile(filepath);
      return;
    }

    if (((len > 5) && (boost::iequals(filepath.substr(len - 5, 5), ".jpeg")))
	|| ((len > 4) && (boost::iequals(filepath.substr(len - 4, 4), ".jpg")))) {
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
