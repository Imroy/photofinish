/*
	Copyright 2012 Ian Tester

	This file is part of Photo Finish.

	Photo Finish is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Photo Finish is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Photo Finish.  If not, see <http://www.gnu.org/licenses/>.
*/
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
    ImageFile(const ImageFile& other);
    ~ImageFile();

    const Image& read(void);
    void write(const Image& img, const Destination &d);
  };

}

#endif // __IMAGEFILE_HH__
