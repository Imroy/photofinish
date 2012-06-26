#ifndef __IMAGEFILE_HH__
#define __IMAGEFILE_HH__

#include <string>
#include <lcms2.h>
#include "Image.hh"
#include "Destination.hh"

#define IMAGE_TYPE (FLOAT_SH(1)|COLORSPACE_SH(PT_Lab)|CHANNELS_SH(3)|BYTES_SH(sizeof(SAMPLE) & 0x07))

class _ImageFile {
protected:
  const string _filepath;

public:
  _ImageFile(const string filepath) :
    _filepath(filepath)
  {}
  virtual ~_ImageFile() {}

  virtual Image* read(void) = 0;
  virtual bool write(Image* i, const Destination &d) = 0;
  inline bool write(Image& i, const Destination &d) { return write(&i, d); }
};

class PNGFile : public _ImageFile {
private:

public:
  PNGFile(const string filepath);

  Image* read(void);
  bool write(Image* img, const Destination &d);
};

class JPEGFile : public _ImageFile {
private:

public:
  JPEGFile(const string filepath);

  Image* read(void);
  bool write(Image* img, const Destination &d);
};

// Factory function
_ImageFile* ImageFile(const string filepath);

#endif // __IMAGEFILE_HH__
