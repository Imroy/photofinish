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
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <tiffio.h>
#include <tiffio.hxx>
#include "ImageFile.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  TIFFfile::TIFFfile(const fs::path filepath) :
    ImageFile(filepath)
  {}

  Image::ptr TIFFfile::read(void) const {
    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream fb(_filepath, std::ios_base::in);
    if (fb.fail())
      throw FileOpenError(_filepath.native());

    TIFF *tiff = TIFFStreamOpen("", &fb);
    if (tiff == NULL)
      throw FileOpenError(_filepath.native());

    uint32 width, height;
    TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
    std::cerr << "\tImage is " << width << "x" << height << std::endl;
    Image::ptr img(new Image(width, height));

    uint16 bit_depth, photometric;
    TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bit_depth);
    std::cerr << "\tImage has a depth of " << bit_depth << std::endl;
    cmsUInt32Number cmsType;
    if (TIFFGetField(tiff, TIFFTAG_PHOTOMETRIC, &photometric) == 1) {
      switch (photometric) {
      case PHOTOMETRIC_MINISBLACK:
	cmsType = COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(bit_depth >> 3);
	img->set_greyscale();
	break;
      case PHOTOMETRIC_RGB:
	cmsType = COLORSPACE_SH(PT_RGB) | DOSWAP_SH(1) | CHANNELS_SH(3) | BYTES_SH(bit_depth >> 3);
	break;
      default:
	std::cerr << "** unsupported TIF photometric interpretation " << photometric << " **" << std::endl;
	exit(1);
      }
    } else
      std::cerr << "** Could not get photometric interpretation **" << std::endl;

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHPROFILE profile = NULL;

    uint32 profile_len;
    void *profile_data;
    if (TIFFGetField(tiff, TIFFTAG_ICCPROFILE, &profile_len, &profile_data) == 1) {
      std::cerr << "\tImage has ICC profile." << std::endl;
      profile = cmsOpenProfileFromMem(profile_data, profile_len);
    }
    if (profile == NULL) {
      if (T_COLORSPACE(cmsType) == PT_RGB) {
	std::cerr << "\tUsing default sRGB profile." << std::endl;
	profile = cmsCreate_sRGBProfile();
      } else {
	std::cerr << "\tUsing default greyscale profile." << std::endl;
	cmsToneCurve *gamma = cmsBuildGamma(NULL, 2.2);
	profile = cmsCreateGrayProfile(cmsD50_xyY(), gamma);
	cmsFreeToneCurve(gamma);
      }
    }

    cmsHTRANSFORM transform = cmsCreateTransform(profile, cmsType,
						 lab, IMAGE_TYPE,
						 INTENT_PERCEPTUAL, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tiff));
    for (unsigned int y = 0; y < height; y++) {
      TIFFReadScanline(tiff, buf, y);

      cmsDoTransform(transform, buf, img->row(y), width);
    }
    _TIFFfree(buf);

    TIFFFlush(tiff);
    fb.close();

    return img;
  }

  void TIFFfile::write(Image::ptr img, const Destination &dest, const Tags &tags) const {
  }

}
