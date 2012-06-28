#ifndef __IMAGEFILE_HH__
#define __IMAGEFILE_HH__

#include <string>
#include <lcms2.h>
#include "Image.hh"
#include "Destination.hh"
#include "Exception.hh"

#define IMAGE_TYPE (FLOAT_SH(1)|COLORSPACE_SH(PT_Lab)|CHANNELS_SH(3)|BYTES_SH(sizeof(SAMPLE) & 0x07))

namespace PhotoFinish {

  class _ImageFile {
  protected:
    const std::string _filepath;

  public:
    _ImageFile(const std::string filepath) :
      _filepath(filepath)
    {}
    virtual ~_ImageFile() {}

    virtual const Image& read(void) = 0;
    virtual void write(const Image& img, const Destination &d) = 0;
  };

  class PNGFile : public _ImageFile {
  private:

  public:
    PNGFile(const std::string filepath);

    const Image& read(void);
    void write(const Image& img, const Destination &d);
  };

  class JPEGFile : public _ImageFile {
  private:

  public:
    JPEGFile(const std::string filepath);

    const Image& read(void);
    void write(const Image& img, const Destination &d);
  };

  // Factory function
  _ImageFile* ImageFile(const std::string filepath) throw(UnknownFileType);

}

#endif // __IMAGEFILE_HH__
