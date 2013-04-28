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
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "ImageFile.hh"
#include "Exception.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  ImageFile::ImageFile(const fs::path filepath) :
    _filepath(filepath),
    _is_open(false)
  {}

  ImageFile::ptr ImageFile::create(const fs::path filepath) throw(UnknownFileType) {
    std::string ext = filepath.extension().generic_string().substr(1);
#ifdef HAZ_PNG
    if (boost::iequals(ext, "png"))
      return std::make_shared<PNGfile>(filepath);
#endif

#ifdef HAZ_JPEG
    if (boost::iequals(ext, "jpeg") || boost::iequals(ext, "jpg"))
      return std::make_shared<JPEGfile>(filepath);
#endif

#ifdef HAZ_TIFF
    if (boost::iequals(ext, "tiff") || boost::iequals(ext, "tif"))
      return std::make_shared<TIFFfile>(filepath);
#endif

#ifdef HAZ_JP2
    if (boost::iequals(ext, "jp2"))
      return std::make_shared<JP2file>(filepath);
#endif

    throw UnknownFileType(filepath.generic_string());
  }

  ImageFile::ptr ImageFile::create(fs::path filepath, const std::string format) throw(UnknownFileType) {
#ifdef HAZ_PNG
    if (boost::iequals(format, "png"))
      return std::make_shared<PNGfile>(filepath.replace_extension(".png"));
#endif

#ifdef HAZ_JPEG
    if (boost::iequals(format, "jpeg")
	|| boost::iequals(format, "jpg"))
      return std::make_shared<JPEGfile>(filepath.replace_extension(".jpeg"));
#endif

#ifdef HAZ_TIFF
    if (boost::iequals(format, "tiff")
	|| boost::iequals(format, "tif"))
      return std::make_shared<TIFFfile>(filepath.replace_extension(".tiff"));
#endif

#ifdef HAZ_JP2
    if (boost::iequals(format, "jp2"))
      return std::make_shared<JP2file>(filepath.replace_extension(".jp2"));
#endif

    if (boost::iequals(format, "sol"))
      return std::make_shared<SOLfile>(filepath.replace_extension(".bin"));

    throw UnknownFileType(format);
  }

  cmsHPROFILE ImageFile::default_profile(cmsUInt32Number cmsType) {
    cmsHPROFILE profile = NULL;
    switch (T_COLORSPACE(cmsType)) {
    case PT_RGB:
      std::cerr << "\tUsing default sRGB profile." << std::endl;
      profile = cmsCreate_sRGBProfile();
      break;

    case PT_GRAY:
      std::cerr << "\tUsing default greyscale profile." << std::endl;
      // Build a greyscale profile with the same gamma curve and white point as sRGB.
      // Copied from cmsCreate_sRGBProfileTHR() and Build_sRGBGamma().
      {
	cmsCIExyY D65;
	cmsWhitePointFromTemp(&D65, 6504);
	double Parameters[5] = {
	  2.4,			// Gamma
	  1.0 / 1.055,		// a
	  0.055 / 1.055,	// b
	  1.0 / 12.92,		// c
	  0.04045,		// d
	};
	// y = (x >= d ? (a*x + b)^Gamma : c*x)
	cmsToneCurve *gamma = cmsBuildParametricToneCurve(NULL, 4, Parameters);
	profile = cmsCreateGrayProfile(&D65, gamma);
	cmsFreeToneCurve(gamma);

	cmsMLU *DescriptionMLU = cmsMLUalloc(NULL, 1);
	if (DescriptionMLU != NULL) {
	  if (cmsMLUsetWide(DescriptionMLU,  "en", "AU", L"sGrey built-in"))
	    cmsWriteTag(profile, cmsSigProfileDescriptionTag,  DescriptionMLU);
	  cmsMLUfree(DescriptionMLU);
	}
      }
      break;

    default:
      std::cerr << "** Cannot assign a default profile for colour space " << T_COLORSPACE(cmsType) << " **" << std::endl;
    }

    return profile;
  }

  cmsHPROFILE ImageFile::get_and_embed_profile(Destination::ptr dest, cmsUInt32Number cmsType, cmsUInt32Number intent) {
    cmsHPROFILE profile = NULL;
    std::string profile_name;
    unsigned char *profile_data = NULL;
    unsigned int profile_len = 0;

    if (dest->profile()) {
      profile = dest->profile()->profile();
      profile_name = dest->profile()->name();
      if (dest->profile()->has_data()) {
	profile_data = (unsigned char*)dest->profile()->data();
	profile_len = dest->profile()->data_size();
      }
    } else {
      profile = this->default_profile(cmsType);
      if (T_COLORSPACE(cmsType) == PT_GRAY) {
	profile_name = "sGrey";
	this->mark_sGrey(intent);
      } else {
	profile_name = "sRGB";
	this->mark_sRGB(intent);
      }
    }

    if (profile_data == NULL) {
      cmsSaveProfileToMem(profile, NULL, &profile_len);
      if (profile_len > 0) {
	profile_data = (unsigned char*)malloc(profile_len);
	cmsSaveProfileToMem(profile, profile_data, &profile_len);
      }
    }

    this->embed_icc(profile_name, profile_data, profile_len);
    return profile;
  }

  void ImageFile::add_variables(Destination::ptr dest, multihash& vars) {
    std::string format = dest->format().get();
    if (boost::iequals(format, "jpeg")
	|| boost::iequals(format, "jpg"))
      const_cast<D_JPEG&>(dest->jpeg()).add_variables(vars);

    if (boost::iequals(format, "tiff")
	|| boost::iequals(format, "tif"))
      const_cast<D_TIFF&>(dest->tiff()).add_variables(vars);

    if (boost::iequals(format, "jp2"))
      const_cast<D_JP2&>(dest->jp2()).add_variables(vars);
  }

  Image::ptr ImageFile::read(void) {
    auto temp = std::make_shared<Destination>();
    return this->read(temp);
  }

}
