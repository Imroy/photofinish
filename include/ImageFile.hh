#ifndef __IMAGEFILE_HH__
#define __IMAGEFILE_HH__

#include <string>
#include <lcms2.h>
#include <memory>
#include <boost/filesystem.hpp>
#include "Image.hh"
#include "Destination.hh"
#include "Exception.hh"

#define IMAGE_TYPE (FLOAT_SH(1)|COLORSPACE_SH(PT_Lab)|CHANNELS_SH(3)|BYTES_SH(sizeof(SAMPLE) & 0x07))

namespace fs = boost::filesystem;

namespace PhotoFinish {

  class Base_ImageFile {
  protected:
    const fs::path _filepath;

  public:
    Base_ImageFile() :
      _filepath()
    {}
    Base_ImageFile(const fs::path filepath) :
      _filepath(filepath)
    {}

    virtual Image::ptr read(void) const = 0;
    virtual void write(Image::ptr img, const Destination &d) const = 0;
  };

  class PNGFile : public Base_ImageFile {
  private:

  public:
    PNGFile(const fs::path filepath);

    Image::ptr read(void) const;
    void write(Image::ptr img, const Destination &d) const;
  };

  class JPEGFile : public Base_ImageFile {
  private:

  public:
    JPEGFile(const fs::path filepath);

    Image::ptr read(void) const;
    void write(Image::ptr img, const Destination &d) const;
  };

  // Factory/wrapper class
  class ImageFile : public Base_ImageFile {
  private:
    typedef std::shared_ptr<Base_ImageFile> ptr;

    ptr _imagefile;

  public:
    ImageFile();
    ImageFile(const fs::path filepath) throw(UnknownFileType);
    ImageFile(fs::path filepath, const std::string format) throw(UnknownFileType);

    Image::ptr read(void) const;
    void write(Image::ptr img, const Destination &d) const;
  };

}

#endif // __IMAGEFILE_HH__
