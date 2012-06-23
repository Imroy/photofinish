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

  virtual Image* read(void) = 0;
  virtual bool write(Image* i) = 0;
  inline bool write(Image& i) { return write(&i); }
};

class PNGFile : private _ImageFile {
private:
  int _bit_depth;
  cmsHPROFILE _profile;
  bool _own_profile;
  cmsUInt32Number _intent;

public:
  PNGFile(const char* filepath);
  ~PNGFile();

  bool set_bit_depth(int bit_depth);
  bool set_profile(cmsHPROFILE profile, cmsUInt32Number intent);

  Image* read(void);
  bool write(Image* img);
};

class JPEGFile : private _ImageFile {
private:

public:
  JPEGFile(const char* filepath);

  bool set_bit_depth(int bit_depth);
  bool set_profile(cmsHPROFILE profile, cmsUInt32Number intent);

  Image* read(void);
  bool write(Image* img);
};

// Factory function
_ImageFile* ImageFile(const char* filepath);

#endif // __IMAGEFILE_HH__
