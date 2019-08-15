/*
	Copyright 2014 Ian Tester

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
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <tiffio.h>
#include <tiffio.hxx>
#include "ImageFile.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  TIFFreader::TIFFreader(const fs::path filepath) :
    ImageReader(filepath)
  {}

#define TIFFcheck(x) if ((rc = TIFF##x) != 1) throw LibraryError("libtiff", "TIFF" #x " returned " + rc)

  Image::ptr TIFFreader::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream fb(_filepath, std::ios_base::in);
    if (fb.fail())
      throw FileOpenError(_filepath.native());

    TIFF *tiff = TIFFStreamOpen("", &fb);
    if (tiff == nullptr)
      throw FileOpenError(_filepath.native());

    int rc;
    uint32 width, height;
    TIFFcheck(GetField(tiff, TIFFTAG_IMAGEWIDTH, &width));
    TIFFcheck(GetField(tiff, TIFFTAG_IMAGELENGTH, &height));
    std::cerr << "\tImage is " << width << "×" << height << std::endl;

    uint16 bit_depth, channels, photometric;
    TIFFcheck(GetField(tiff, TIFFTAG_BITSPERSAMPLE, &bit_depth));
    dest->set_depth(bit_depth);
    std::cerr << "\tImage has a depth of " << bit_depth << std::endl;

    TIFFcheck(GetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &channels));

    CMS::Format format;
    switch (bit_depth >> 3) {
    case 1: format.set_8bit();
      break;

    case 2: format.set_16bit();
      break;

    case 4: format.set_32bit();
      break;

    default:
      std::cerr << "** Unknown depth " << (bit_depth >> 3) << " **" << std::endl;
      throw LibraryError("libTIFF", "depth");
    }

    {
      uint16 extra_count, *extra_types;
      if (TIFFGetField(tiff, TIFFTAG_EXTRASAMPLES, &extra_count, &extra_types) == 1) {
	format.set_extra_channels(extra_count & 0x07);
	for (int i = 0; i < extra_count; i++) {
	  std::cerr << "\tImage has an ";
	  switch (extra_types[i]) {
	  case EXTRASAMPLE_UNSPECIFIED: std::cerr << "unspecified ";
	    break;
	  case EXTRASAMPLE_ASSOCALPHA: std::cerr << "associated alpha ";
	    format.set_premult_alpha();
	    break;
	  case EXTRASAMPLE_UNASSALPHA: std::cerr << "unassociated alpha ";
	    format.unset_premult_alpha();
	    break;
	  default: std::cerr << "unknown ";
	  }
	  std::cerr << "extra channel." << std::endl;
	}
	channels -= extra_count;
      }
    }
    std::cerr << "\tImage has " << channels << " channels." << std::endl;

    TIFFcheck(GetField(tiff, TIFFTAG_PHOTOMETRIC, &photometric));
    switch (photometric) {
    case PHOTOMETRIC_MINISWHITE:
      format.set_vanilla();
    case PHOTOMETRIC_MINISBLACK:
      format.set_colour_model(CMS::ColourModel::Greyscale, channels);
      break;

    case PHOTOMETRIC_RGB:
      format.set_colour_model(CMS::ColourModel::RGB, channels);
      break;

    case PHOTOMETRIC_SEPARATED:
      format.set_colour_model(CMS::ColourModel::CMYK, channels);
      break;

    default:
      std::cerr << "** unsupported TIFF photometric interpretation " << photometric << " **" << std::endl;
      exit(1);
    }
    auto img = std::make_shared<Image>(width, height, format);

    {
      float xres, yres;
      short unsigned int resunit;
      if ((TIFFGetField(tiff, TIFFTAG_XRESOLUTION, &xres) == 1)
	  && (TIFFGetField(tiff, TIFFTAG_YRESOLUTION, &yres) == 1)
	  && (TIFFGetField(tiff, TIFFTAG_RESOLUTIONUNIT, &resunit) == 1)) {
	switch (resunit) {
	case RESUNIT_INCH:
	  img->set_resolution(xres, yres);
	  break;
	case RESUNIT_CENTIMETER:
	  img->set_resolution((double)xres * 2.54, (double)yres * 2.54);
	  break;
	default:
	  std::cerr << "** unknown resolution unit " << resunit << " **" << std::endl;
	}
	if (img->xres().defined() && img->yres().defined())
	  std::cerr << "\tImage has resolution of " << img->xres() << "×" << img->yres() << " PPI." << std::endl;
      }
    }

    {
      uint32 profile_len;
      unsigned char *profile_data;
      if (TIFFGetField(tiff, TIFFTAG_ICCPROFILE, &profile_len, &profile_data) == 1) {
	CMS::Profile::ptr profile = std::make_shared<CMS::Profile>(profile_data, profile_len);
	unsigned char *data_copy = new unsigned char[profile_len];
	memcpy(data_copy, profile_data, profile_len);

	std::string profile_name = profile->description("en", "");
	if (profile_name.length() > 0)
	  dest->set_profile(profile_name, data_copy, profile_len);
	else
	  dest->set_profile("TIFFTAG_ICCPROFILE", data_copy, profile_len);
	img->set_profile(profile);
	std::cerr << "\tRead embedded profile \"" << dest->profile()->name().get() << "\" (" << profile_len << " bytes)" << std::endl;
      }
    }

    std::cerr << "\tReading TIFF image..." << std::endl;
    for (unsigned int y = 0; y < height; y++) {
      img->check_rowdata_alloc(y);
      TIFFcheck(ReadScanline(tiff, img->row(y), y));
      std::cerr << "\r\tRead " << (y + 1) << " of " << height << " rows";
    }
    std::cerr << "\r\tRead " << height << " of " << height << " rows." << std::endl;

    TIFFClose(tiff);
    fb.close();
    _is_open = false;

    std::cerr << "\tExtracting tags..." << std::endl;
    extract_tags(img);

    std::cerr << "Done." << std::endl;
    return img;
  }

}
