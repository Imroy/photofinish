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

#define TIFFcheck(x) if ((rc = TIFF##x) != 1) throw LibraryError("libtiff", "TIFF" #x " returned " + rc)

  TIFFwriter::TIFFwriter(std::ostream* os, Destination::ptr dest, bool close_on_end) :
    ImageWriter(os, dest, close_on_end),
    _tiff(NULL)
  {
    _tiff = TIFFStreamOpen("", os);
    if (_tiff == NULL)
      throw FileOpenError("stream");
  }

  void TIFFwriter::receive_image_header(ImageHeader::ptr header) {
    ImageWriter::receive_image_header(header);

    int rc;
    TIFFcheck(SetField(_tiff, TIFFTAG_SUBFILETYPE, 0));
    TIFFcheck(SetField(_tiff, TIFFTAG_IMAGEWIDTH, header->width()));
    TIFFcheck(SetField(_tiff, TIFFTAG_IMAGELENGTH, header->height()));
    TIFFcheck(SetField(_tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT));
    TIFFcheck(SetField(_tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG));

    {
      bool compression_set = false;
      if (_dest->tiff().defined()) {
	// Note that EXIV2 appears to overwrite the artist and copyright fields with its own information (if it's set)
	if (_dest->tiff().artist().defined())
	  TIFFcheck(SetField(_tiff, TIFFTAG_ARTIST, _dest->tiff().artist().get().c_str()));
	if (_dest->tiff().copyright().defined())
	  TIFFcheck(SetField(_tiff, TIFFTAG_COPYRIGHT, _dest->tiff().copyright().get().c_str()));

	if (_dest->tiff().compression().defined()) {
	  std::string compression = _dest->tiff().compression().get();
	  if (boost::iequals(compression, "deflate")) {
	    TIFFcheck(SetField(_tiff, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE));
	    TIFFcheck(SetField(_tiff, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL));
	    compression_set = true;
	  } else if (boost::iequals(compression, "lzw")) {
	    TIFFcheck(SetField(_tiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW));
	    TIFFcheck(SetField(_tiff, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL));
	    compression_set = true;
	  }
	}
      }
      // Default to no compression (best compatibility and TIFF's compression doesn't make much difference)
      if (!compression_set)
	TIFFcheck(SetField(_tiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE));
    }

    // For some reason none of this information shows up in the written TIFF file
    /*
    TIFFcheck(SetField(_tiff, TIFFTAG_SOFTWARE, "PhotoFinish"));
    TIFFcheck(SetField(_tiff, TIFFTAG_DOCUMENTNAME, _filepath.filename().c_str()));
    {
      char *hostname = (char*)malloc(101);
      if (gethostname(hostname, 100) == 0) {
	std::cerr << "\tSetting hostcomputer tag to \"" << hostname << "\"" << std::endl;
	TIFFcheck(SetField(_tiff, TIFFTAG_HOSTCOMPUTER, hostname));
      }
    }
    {
      time_t t = time(NULL);
      tm *lt = localtime(&t);
      if (lt != NULL) {
	char *datetime = (char*)malloc(20);
	strftime(datetime, 20, "%Y:%m:%d %H:%M:%S", lt);
	std::cerr << "\tSetting datetime tag to \"" << datetime << "\"" << std::endl;
	TIFFcheck(SetField(_tiff, TIFFTAG_DATETIME, datetime));
      }
    }
    */

    if (header->xres().defined()) {
      TIFFcheck(SetField (_tiff, TIFFTAG_XRESOLUTION, (float)header->xres()));
      TIFFcheck(SetField (_tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH));
    }
    if (header->yres().defined()) {
      TIFFcheck(SetField (_tiff, TIFFTAG_YRESOLUTION, (float)header->yres()));
      TIFFcheck(SetField (_tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH));
    }

    switch (T_COLORSPACE(header->cmsType())) {
    case PT_RGB:
      TIFFcheck(SetField(_tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB));
      break;

    case PT_GRAY:
      TIFFcheck(SetField(_tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK));
      break;

    default:
      break;
    }
    TIFFcheck(SetField(_tiff, TIFFTAG_SAMPLESPERPIXEL, T_CHANNELS(header->cmsType())));

    int depth = T_BYTES(header->cmsType()) * 8;
    TIFFcheck(SetField(_tiff, TIFFTAG_BITSPERSAMPLE, depth));

    cmsUInt32Number intent = INTENT_PERCEPTUAL;	// Default value
    if (_dest->intent().defined())
      intent = _dest->intent();

    this->get_and_embed_profile(header->profile(), header->cmsType(), intent);
  }

  void TIFFwriter::mark_sGrey(cmsUInt32Number intent) const {}

  void TIFFwriter::mark_sRGB(cmsUInt32Number intent) const {}

  void TIFFwriter::embed_icc(std::string name, unsigned char *data, unsigned int len) const {
    int rc;
    TIFFcheck(SetField(_tiff, TIFFTAG_ICCPROFILE, len, data));
  }

  void TIFFwriter::_write_row(ImageRow::ptr row) {
    TIFFWriteScanline(_tiff, row->data(), _next_y, 0);
  }

  void TIFFwriter::_finish_writing(void) {
    TIFFClose(_tiff);
    _tiff = NULL;
  }

}
