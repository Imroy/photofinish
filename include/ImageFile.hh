#ifndef __IMAGEFILE_HH__
#define __IMAGEFILE_HH__

#include <lcms2.h>
#include "Image.hh"

class _ImageFile {
protected:
  const char *_filepath;

public:
  _ImageFile(const char* filepath);
  virtual ~_ImageFile();

  virtual bool set_bit_depth(int bit_depth) = 0;
  virtual bool set_profile(cmsHPROFILE profile, cmsUInt32Number intent) = 0;

  virtual Image& read(void) = 0;
  virtual bool write(Image& i) = 0;
};

class PNGFile : private _ImageFile {
private:
  int _bit_depth;
  cmsHPROFILE _profile;
  cmsUInt32Number _intent;

public:
  PNGFile(const char* filepath);
  ~PNGFile();

  bool set_bit_depth(int bit_depth);
  bool set_profile(cmsHPROFILE profile, cmsUInt32Number intent);

  Image& read(void);
  bool write(Image& img);
};

class JPEGFile : private _ImageFile {
private:

public:
  JPEGFile(const char* filepath);

  bool set_bit_depth(int bit_depth);
  bool set_profile(cmsHPROFILE profile, cmsUInt32Number intent);

  Image& read(void);
  bool write(Image& img);
};

// Factory function
_ImageFile* ImageFile(const char* filepath);

#endif // __IMAGEFILE_HH__
