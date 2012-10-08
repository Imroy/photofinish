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

  TIFFreader::TIFFreader(std::istream* is) :
    ImageReader(is),
    _tiff(NULL),
    _next_y(0)
  {
    _tiff = TIFFStreamOpen("", is);
    if (_tiff == NULL)
      throw FileOpenError("stream");
  }

  void TIFFreader::do_work(void) {
    int rc;
    switch (_read_state) {
    case 0:
      {
	uint32 width, height;
	TIFFcheck(GetField(_tiff, TIFFTAG_IMAGEWIDTH, &width));
	TIFFcheck(GetField(_tiff, TIFFTAG_IMAGELENGTH, &height));
	ImageHeader::ptr header(new ImageHeader(width, height));

	uint16 bit_depth, channels, photometric;
	TIFFcheck(GetField(_tiff, TIFFTAG_BITSPERSAMPLE, &bit_depth));
	TIFFcheck(GetField(_tiff, TIFFTAG_SAMPLESPERPIXEL, &channels));

	cmsUInt32Number cmsType = BYTES_SH(bit_depth >> 3);
	{
	  uint16 extra_count, *extra_types;
	  if (TIFFGetField(_tiff, TIFFTAG_EXTRASAMPLES, &extra_count, &extra_types) == 1) {
	    cmsType |= EXTRA_SH(extra_count & 0x07);
	    for (int i = 0; i < extra_count; i++) {
	      std::cerr << "\tImage has an ";
	      switch (extra_types[i]) {
	      case EXTRASAMPLE_UNSPECIFIED: std::cerr << "unspecified ";
		break;
	      case EXTRASAMPLE_ASSOCALPHA: std::cerr << "associated alpha ";
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
	cmsType |= CHANNELS_SH(channels);

	TIFFcheck(GetField(_tiff, TIFFTAG_PHOTOMETRIC, &photometric));
	switch (photometric) {
	case PHOTOMETRIC_MINISWHITE:
	  cmsType |= FLAVOR_SH(1);
	case PHOTOMETRIC_MINISBLACK:
	  cmsType |= COLORSPACE_SH(PT_GRAY);
	  break;

	case PHOTOMETRIC_RGB:
	  cmsType |= COLORSPACE_SH(PT_RGB);
	  break;

	case PHOTOMETRIC_SEPARATED:
	  cmsType |= COLORSPACE_SH(PT_CMYK);
	  break;

	default:
	  std::cerr << "** unsupported TIFF photometric interpretation " << photometric << " **" << std::endl;
	  exit(1);
	}
	header->set_cmsType(cmsType);

	{
	  float xres, yres;
	  short unsigned int resunit;
	  if ((TIFFGetField(_tiff, TIFFTAG_XRESOLUTION, &xres) == 1)
	      && (TIFFGetField(_tiff, TIFFTAG_YRESOLUTION, &yres) == 1)
	      && (TIFFGetField(_tiff, TIFFTAG_RESOLUTIONUNIT, &resunit) == 1)) {
	    switch (resunit) {
	    case RESUNIT_INCH:
	      header->set_resolution(xres, yres);
	      break;
	    case RESUNIT_CENTIMETER:
	      header->set_resolution((double)xres * 2.54, (double)yres * 2.54);
	      break;
	    default:
	      std::cerr << "** unknown resolution unit " << resunit << " **" << std::endl;
	    }
	  }
	}

	cmsHPROFILE profile = NULL;

	uint32 profile_len;
	void *profile_data;
	if (TIFFGetField(_tiff, TIFFTAG_ICCPROFILE, &profile_len, &profile_data) == 1) {
	  profile = cmsOpenProfileFromMem(profile_data, profile_len);
	  header->set_profile(profile);
	}
	if (profile == NULL)
	  header->set_profile(default_profile(cmsType));

	this->_send_image_header(header);
      }
      _read_state = 1;
      break;

    case 1:
      {
	tdata_t buffer = _TIFFmalloc(TIFFScanlineSize(_tiff));
	TIFFcheck(ReadScanline(_tiff, buffer, _next_y));
	ImageRow::ptr row(new ImageRow(_next_y, buffer));
	_next_y++;
	this->_send_image_row(row);
      }
      break;

    case 2:
      TIFFClose(_tiff);
      _tiff = NULL;
      _read_state = 99;
      this->_send_image_end();
      break;

    default:
      break;
    }
  }

}
