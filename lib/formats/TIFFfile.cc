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
#include <boost/filesystem/fstream.hpp>
#include <tiffio.h>
#include <tiffio.hxx>
#include "ImageFile.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  TIFFfile::TIFFfile(const fs::path filepath) :
    ImageFile(filepath)
  {}

#define TIFFcheck(x) if ((rc = TIFF##x) != 1) throw LibraryError("libtiff", "TIFF" #x " returned " + rc)

  // Pre-multiply the colour values with the alpha
  template <typename P>
  void alpha_mult(const P* row, unsigned int width, unsigned char channels) {
    P *p = const_cast<P*>(row);
    for (unsigned int x = 0; x < width; x++, p += channels + 1) {
      if (p[channels] == 0)
	continue;
      for (unsigned char c = 0; c < channels; c++)
	p[c] = (p[c] * maxval<P>()) / p[channels];
    }
  }

  Image::ptr TIFFfile::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream fb(_filepath, std::ios_base::in);
    if (fb.fail())
      throw FileOpenError(_filepath.native());

    TIFF *tiff = TIFFStreamOpen("", &fb);
    if (tiff == NULL)
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

    bool need_alpha_mult = false;
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
	    need_alpha_mult = true;
	    break;
	  case EXTRASAMPLE_UNASSALPHA: std::cerr << "unassociated alpha ";
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
      void *profile_data;
      if (TIFFGetField(tiff, TIFFTAG_ICCPROFILE, &profile_len, &profile_data) == 1) {
	CMS::Profile::ptr profile = std::make_shared<CMS::Profile>(profile_data, profile_len);
	void *data_copy = malloc(profile_len);
	memcpy(data_copy, profile_data, profile_len);

	std::string profile_name = profile->read_info(cmsInfoDescription, "en", cmsNoCountry);
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
      if (need_alpha_mult)
	switch (bit_depth) {
	case 8:
	  alpha_mult<unsigned char>(img->row<unsigned char>(y), width, channels);
	  break;

	case 16:
	  alpha_mult<unsigned short int>(img->row<short unsigned int>(y), width, channels);
	  break;

	case 32:
	  alpha_mult<unsigned int>(img->row<unsigned int>(y), width, channels);
	  break;

	}
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

  CMS::Format TIFFfile::preferred_format(CMS::Format format) {
    if ((format.colour_model() != CMS::ColourModel::Greyscale)
	&& (format.colour_model() != CMS::ColourModel::CMYK)) {
      format.set_colour_model(CMS::ColourModel::RGB);
    }

    format.set_planar(false);

    if (!format.is_8bit() && !format.is_16bit())
      format.set_16bit();

    return format;
  }

  void TIFFfile::write(Image::ptr img, Destination::ptr dest, bool can_free) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ofstream fb;
    fb.open(_filepath, std::ios_base::out);

    TIFF *tiff = TIFFStreamOpen("", &fb);
    if (tiff == NULL)
      throw FileOpenError(_filepath.native());

    int rc;

    TIFFcheck(SetField(tiff, TIFFTAG_SUBFILETYPE, 0));
    TIFFcheck(SetField(tiff, TIFFTAG_IMAGEWIDTH, img->width()));
    TIFFcheck(SetField(tiff, TIFFTAG_IMAGELENGTH, img->height()));
    TIFFcheck(SetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT));
    TIFFcheck(SetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG));

    {
      bool compression_set = false;
      if (dest->tiff().defined()) {
	// Note that EXIV2 appears to overwrite the artist and copyright fields with its own information (if it's set)
	if (dest->tiff().artist().defined())
	  TIFFcheck(SetField(tiff, TIFFTAG_ARTIST, dest->tiff().artist().get().c_str()));
	if (dest->tiff().copyright().defined())
	  TIFFcheck(SetField(tiff, TIFFTAG_COPYRIGHT, dest->tiff().copyright().get().c_str()));

	if (dest->tiff().compression().defined()) {
	  std::string compression = dest->tiff().compression().get();
	  if (boost::iequals(compression, "deflate")) {
	    TIFFcheck(SetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE));
	    TIFFcheck(SetField(tiff, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL));
	    compression_set = true;
	  } else if (boost::iequals(compression, "lzw")) {
	    TIFFcheck(SetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW));
	    TIFFcheck(SetField(tiff, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL));
	    compression_set = true;
	  }
	}
      }
      // Default to no compression (best compatibility and TIFF's compression doesn't make much difference)
      if (!compression_set)
	TIFFcheck(SetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE));
    }

    // For some reason none of this information shows up in the written TIFF file
    /*
    TIFFcheck(SetField(tiff, TIFFTAG_SOFTWARE, "PhotoFinish"));
    TIFFcheck(SetField(tiff, TIFFTAG_DOCUMENTNAME, _filepath.filename().c_str()));
    {
      char *hostname = (char*)malloc(101);
      if (gethostname(hostname, 100) == 0) {
	std::cerr << "\tSetting hostcomputer tag to \"" << hostname << "\"" << std::endl;
	TIFFcheck(SetField(tiff, TIFFTAG_HOSTCOMPUTER, hostname));
      }
    }
    {
      time_t t = time(NULL);
      tm *lt = localtime(&t);
      if (lt != NULL) {
	char *datetime = (char*)malloc(20);
	strftime(datetime, 20, "%Y:%m:%d %H:%M:%S", lt);
	std::cerr << "\tSetting datetime tag to \"" << datetime << "\"" << std::endl;
	TIFFcheck(SetField(tiff, TIFFTAG_DATETIME, datetime));
      }
    }
    */

    if (img->xres().defined()) {
      TIFFcheck(SetField (tiff, TIFFTAG_XRESOLUTION, (float)img->xres()));
      TIFFcheck(SetField (tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH));
    }
    if (img->yres().defined()) {
      TIFFcheck(SetField (tiff, TIFFTAG_YRESOLUTION, (float)img->yres()));
      TIFFcheck(SetField (tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH));
    }

    CMS::Format format = img->format();
    switch (format.colour_model()) {
    case CMS::ColourModel::RGB:
      TIFFcheck(SetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB));
      break;

    case CMS::ColourModel::Greyscale:
      TIFFcheck(SetField(tiff, TIFFTAG_PHOTOMETRIC, format.is_vanilla() ? PHOTOMETRIC_MINISWHITE : PHOTOMETRIC_MINISBLACK));
      break;

    case CMS::ColourModel::CMYK:
      TIFFcheck(SetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_SEPARATED));
      break;

    default:
      break;
    }
    TIFFcheck(SetField(tiff, TIFFTAG_SAMPLESPERPIXEL, format.total_channels()));
    if (format.extra_channels()) {
      uint16 extra_types[1] = { EXTRASAMPLE_ASSOCALPHA };
      TIFFcheck(SetField(tiff, TIFFTAG_EXTRASAMPLES, 1, extra_types));
    }

    TIFFcheck(SetField(tiff, TIFFTAG_BITSPERSAMPLE, format.bytes_per_channel() << 3));

    {
      void *profile_data = NULL;
      unsigned int profile_len = 0;
      img->profile()->save_to_mem(profile_data, profile_len);
      if (profile_len > 0) {
	std::cerr << "\tEmbedding profile (" << profile_len << " bytes)." << std::endl;
	TIFFcheck(SetField(tiff, TIFFTAG_ICCPROFILE, profile_len, profile_data));
      }
    }

    for (unsigned int y = 0; y < img->height(); y++) {
      TIFFWriteScanline(tiff, img->row(y), y, 0);

      if (can_free)
	img->free_row(y);

      std::cerr << "\r\tWritten " << y + 1 << " of " << img->height() << " rows";
    }
    std::cerr << "\r\tWritten " << img->height() << " of " << img->height() << " rows." << std::endl;

    TIFFClose(tiff);
    fb.close();
    _is_open = false;

    std::cerr << "\tEmbedding tags..." << std::endl;
    embed_tags(img);

    std::cerr << "Done." << std::endl;
  }

}
