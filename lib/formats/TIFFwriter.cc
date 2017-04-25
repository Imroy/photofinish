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
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <tiffio.h>
#include <tiffio.hxx>
#include "ImageFile.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  TIFFwriter::TIFFwriter(const fs::path filepath) :
    ImageWriter(filepath)
  {}

#define TIFFcheck(x) if ((rc = TIFF##x) != 1) throw LibraryError("libtiff", "TIFF" #x " returned " + rc)

  CMS::Format TIFFwriter::preferred_format(CMS::Format format) {
    if ((format.colour_model() != CMS::ColourModel::Greyscale)
	&& (format.colour_model() != CMS::ColourModel::CMYK)) {
      format.set_colour_model(CMS::ColourModel::RGB);
    }

    format.set_packed();

    if (!format.is_8bit() && !format.is_16bit())
      format.set_16bit();

    return format;
  }

  void TIFFwriter::write(Image::ptr img, Destination::ptr dest, bool can_free) {
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
      uint16 extra_types[1];
      extra_types[0] = format.is_premult_alpha() ? EXTRASAMPLE_ASSOCALPHA : EXTRASAMPLE_UNASSALPHA;
      TIFFcheck(SetField(tiff, TIFFTAG_EXTRASAMPLES, 1, extra_types));
    }

    TIFFcheck(SetField(tiff, TIFFTAG_BITSPERSAMPLE, format.bytes_per_channel() << 3));

    if (img->has_profile()) {
      unsigned char *profile_data = NULL;
      unsigned int profile_len = 0;
      img->profile()->save_to_mem(profile_data, profile_len);
      if (profile_len > 0) {
	std::cerr << "\tEmbedding profile (" << profile_len << " bytes)." << std::endl;
	TIFFcheck(SetField(tiff, TIFFTAG_ICCPROFILE, profile_len, profile_data));
      }
    }

    for (unsigned int y = 0; y < img->height(); y++) {
      TIFFWriteScanline(tiff, img->row(y)->data(), y, 0);

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
