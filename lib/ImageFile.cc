#include <boost/algorithm/string.hpp>
#include "ImageFile.hh"
#include "Exception.hh"

namespace PhotoFinish {

  _ImageFile* ImageFile(const std::string filepath) throw(UnknownFileType) {
    int len = filepath.length();
    if ((len > 4) && (boost::iequals(filepath.substr(len - 4, 4), ".png")))
      return new PNGFile(filepath);
    if (((len > 5) && (boost::iequals(filepath.substr(len - 5, 5), ".jpeg")))
	|| ((len > 4) && (boost::iequals(filepath.substr(len - 4, 4), ".jpg"))))
      return new JPEGFile(filepath);

    throw UnknownFileType(filepath);
  }

}
