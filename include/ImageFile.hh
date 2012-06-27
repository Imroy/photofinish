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
#include "Image.hh"
#include "Destination.hh"

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

    virtual Image* read(void) = 0;
    virtual bool write(Image* i, const Destination &d) = 0;
    inline bool write(Image& i, const Destination &d) { return write(&i, d); }
  };

  class PNGFile : public _ImageFile {
  private:

  public:
    PNGFile(const std::string filepath);

    Image* read(void);
    bool write(Image* img, const Destination &d);
  };

  class JPEGFile : public _ImageFile {
  private:

  public:
    JPEGFile(const std::string filepath);

    Image* read(void);
    bool write(Image* img, const Destination &d);
  };

  // Factory function
  _ImageFile* ImageFile(const std::string filepath);

}

#endif // __IMAGEFILE_HH__
