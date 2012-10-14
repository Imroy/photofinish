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
#include <omp.h>
#include "ImageFile.hh"
#include "Exception.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  ImageFilepath::ImageFilepath(const fs::path filepath, const std::string format) :
    _filepath(filepath),
    _format(format)
  {}

  ImageFilepath::ImageFilepath(const fs::path filepath) throw(UnknownFileType) :
    _filepath(filepath)
  {
    std::string ext = filepath.extension().generic_string().substr(1);
    bool unknown = true;
#ifdef HAZ_PNG
    if (boost::iequals(ext, "png")) {
      _format = "png";
      unknown = false;
    }
#endif

#ifdef HAZ_JPEG
    if (boost::iequals(ext, "jpeg") || boost::iequals(ext, "jpg")) {
      _format = "jpeg";
      unknown = false;
    }
#endif

#ifdef HAZ_TIFF
    if (boost::iequals(ext, "tiff") || boost::iequals(ext, "tif")) {
      _format = "tiff";
      unknown = false;
    }
#endif

#ifdef HAZ_JP2
    if (boost::iequals(ext, "jp2")) {
      _format = "jp2";
      unknown = false;
    }
#endif

    if (unknown)
      throw UnknownFileType(filepath.generic_string());
  }



  ImageReader::ImageReader(std::istream* is) :
    _is(is),
    _read_state(0),
    _reader_lock((omp_lock_t*)malloc(sizeof(omp_lock_t)))
  {
    omp_init_lock(_reader_lock);
  }

  ImageReader::~ImageReader() {
    omp_destroy_lock(_reader_lock);
    free(_reader_lock);
  }

  ImageReader::ptr ImageReader::open(const ImageFilepath ifp) throw(UnknownFileType) {
#ifdef HAZ_PNG
    if (boost::iequals(ifp.format(), "png"))
      return ImageReader::ptr(new PNGreader(new fs::ifstream(ifp.filepath())));
#endif

#ifdef HAZ_JPEG
    if (boost::iequals(ifp.format(), "jpeg"))
      return ImageReader::ptr(new JPEGreader(new fs::ifstream(ifp.filepath())));
#endif

#ifdef HAZ_TIFF
    if (boost::iequals(ifp.format(), "tiff"))
      return ImageReader::ptr(new TIFFreader(new fs::ifstream(ifp.filepath())));
#endif

#ifdef HAZ_JP2
    if (boost::iequals(ifp.format(), "jp2"))
      return ImageReader::ptr(new JP2reader(new fs::ifstream(ifp.filepath())));
#endif

    throw UnknownFileType(ifp.format());
  }



  ImageWriter::ImageWriter(std::ostream* os, Destination::ptr dest) :
    ImageSink(),
    _os(os),
    _dest(dest)
  {}

  ImageWriter::ptr ImageWriter::open(const ImageFilepath ifp, Destination::ptr dest) throw(UnknownFileType) {
#ifdef HAZ_PNG
    if (boost::iequals(ifp.format(), "png"))
      return ImageWriter::ptr(new PNGwriter(new fs::ofstream(ifp.filepath().replace_extension(".png")), dest));
#endif

#ifdef HAZ_JPEG
    if (boost::iequals(ifp.format(), "jpeg")
	|| boost::iequals(ifp.format(), "jpg"))
      return ImageWriter::ptr(new JPEGwriter(new fs::ofstream(ifp.filepath().replace_extension(".jpeg")), dest));
#endif

#ifdef HAZ_TIFF
    if (boost::iequals(ifp.format(), "tiff")
	|| boost::iequals(ifp.format(), "tif"))
      return ImageWriter::ptr(new TIFFwriter(new fs::ofstream(ifp.filepath().replace_extension(".tiff")), dest));
#endif

#ifdef HAZ_JP2
    if (boost::iequals(ifp.format(), "jp2"))
      return ImageWriter::ptr(new JP2writer(new fs::ofstream(ifp.filepath().replace_extension(".jp2")), dest));
#endif

    if (boost::iequals(ifp.format(), "sol"))
      return ImageWriter::ptr(new SOLwriter(new fs::ofstream(ifp.filepath().replace_extension(".bin")), dest));

    throw UnknownFileType(ifp.format());
  }

  void ImageWriter::receive_image_header(ImageHeader::ptr header) {
    ImageSink::receive_image_header(header);
  }

  void ImageWriter::receive_image_row(ImageRow::ptr row) {
    ImageSink::receive_image_row(row);
  }

  void ImageWriter::receive_image_end(void) {
    ImageSink::receive_image_end();
  }



  cmsHPROFILE default_profile(cmsUInt32Number cmsType) {
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

  void ImageWriter::get_and_embed_profile(cmsHPROFILE profile, cmsUInt32Number cmsType, cmsUInt32Number intent) {
    if (profile == NULL) {
      if (T_COLORSPACE(cmsType) == PT_GRAY)
	this->mark_sGrey(intent);
      else
	this->mark_sRGB(intent);
    } else {
      char *profile_name = NULL;
      unsigned int profile_name_len;
      if ((profile_name_len = cmsGetProfileInfoASCII(profile, cmsInfoDescription, "en", cmsNoCountry, NULL, 0)) > 0) {
        profile_name = (char*)malloc(profile_name_len);
        cmsGetProfileInfoASCII(profile, cmsInfoDescription, "en", cmsNoCountry, profile_name, profile_name_len);
      } else {
        profile_name = (char*)malloc(1);
	profile_name[0] = 0;
      }

      unsigned int profile_len;
      cmsSaveProfileToMem(profile, NULL, &profile_len);
      if (profile_len > 0) {
	unsigned char *profile_data = (unsigned char*)malloc(profile_len);
	cmsSaveProfileToMem(profile, profile_data, &profile_len);
	this->embed_icc(profile_name, profile_data, profile_len);
      }
      if (profile_name != NULL)
        free(profile_name);
    }
  }

  void add_format_variables(Destination::ptr dest, multihash& vars) {
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

}
