#ifndef __IMAGEFILE_HH__
#define __IMAGEFILE_HH__

#include <string>
#include <lcms2.h>
#include <boost/filesystem.hpp>
#include "Image.hh"
#include "Destination.hh"
#include "Exception.hh"

#define IMAGE_TYPE (FLOAT_SH(1)|COLORSPACE_SH(PT_Lab)|CHANNELS_SH(3)|BYTES_SH(sizeof(SAMPLE) & 0x07))

namespace fs = boost::filesystem;

namespace PhotoFinish {

  class _ImageFile {
  protected:
    const fs::path _filepath;

  public:
    _ImageFile() :
      _filepath()
    {}
    _ImageFile(const fs::path filepath) :
      _filepath(filepath)
    {}
    virtual ~_ImageFile() {}

    virtual const Image& read(void) = 0;
    virtual void write(const Image& img, const Destination &d) = 0;
  };

  class PNGFile : public _ImageFile {
  private:

  public:
    PNGFile(const fs::path filepath);

    const Image& read(void);
    void write(const Image& img, const Destination &d);
  };

  class JPEGFile : public _ImageFile {
  private:

  public:
    JPEGFile(const fs::path filepath);

    const Image& read(void);
    void write(const Image& img, const Destination &d);
  };

  // Factory/wrapper class
  class ImageFile : public _ImageFile {
  private:
    _ImageFile *_imagefile;

  public:
    ImageFile();
    ImageFile(const fs::path filepath) throw(UnknownFileType);
    ImageFile(fs::path filepath, const std::string format) throw(UnknownFileType);
    ImageFile(const ImageFile& other);
    ~ImageFile();

    const Image& read(void);
    void write(const Image& img, const Destination &d);
  };

}

#endif // __IMAGEFILE_HH__
