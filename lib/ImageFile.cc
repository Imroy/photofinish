#include <boost/algorithm/string.hpp>
#include "ImageFile.hh"

namespace PhotoFinish {

  _ImageFile* ImageFile(const std::string filepath) {
    int len = filepath.length();
    if ((len > 4) && (boost::iequals(filepath.substr(len - 4, 4), ".png")))
      return (_ImageFile*)new PNGFile(filepath);
    if (((len > 5) && (boost::iequals(filepath.substr(len - 5, 5), ".jpeg")))
	|| ((len > 4) && (boost::iequals(filepath.substr(len - 4, 4), ".jpg"))))
      return (_ImageFile*)new JPEGFile(filepath);

    return NULL;
  }

}
