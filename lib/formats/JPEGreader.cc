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
#include <iostream>
#include <queue>
#include <list>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <string.h>
#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <omp.h>
#include <lcms2.h>
#include "ImageFile.hh"
#include "Image.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  //! Set up a "source manager" on the given JPEG decompression structure to read from an istream
  void jpeg_istream_src(j_decompress_ptr dinfo, std::istream* is);

  //! Free the data structures of the istream source manager
  void jpeg_istream_src_free(j_decompress_ptr dinfo);

  JPEGreader::JPEGreader(std::istream* is) :
    ImageReader(is),
    _dinfo(NULL)
  {
    _dinfo = (jpeg_decompress_struct*)malloc(sizeof(jpeg_decompress_struct));
    jpeg_create_decompress(_dinfo);
    struct jpeg_error_mgr jerr;
    _dinfo->err = jpeg_std_error(&jerr);

    jpeg_istream_src(_dinfo, _is);
  }

  //! Read an ICC profile from APP2 markers in a JPEG file
  cmsHPROFILE jpeg_read_profile(jpeg_decompress_struct*);

  void JPEGreader::do_work(void) {
    if (!this->_test_reader_lock())
      return;

    switch (_read_state) {
    case 0:
      jpeg_save_markers(_dinfo, JPEG_APP0 + 2, 0xFFFF);
      jpeg_read_header(_dinfo, TRUE);

      {
	cmsUInt32Number cmsType = CHANNELS_SH(_dinfo->num_components) | BYTES_SH(1);
	switch (_dinfo->jpeg_color_space) {
	case JCS_GRAYSCALE:
	  cmsType |= COLORSPACE_SH(PT_GRAY);
	  break;

	case JCS_YCbCr:
	  _dinfo->out_color_space = JCS_RGB;
	case JCS_RGB:
	  cmsType |= COLORSPACE_SH(PT_RGB);
	  break;

	case JCS_YCCK:
	  _dinfo->out_color_space = JCS_CMYK;
	case JCS_CMYK:
	  cmsType |= COLORSPACE_SH(PT_CMYK);
	  if (_dinfo->saw_Adobe_marker)
	    cmsType |= FLAVOR_SH(1);
	  break;

	default:
	  std::cerr << "** unsupported JPEG colour space " << _dinfo->jpeg_color_space << " **" << std::endl;
	  exit(1);
	}
	ImageHeader::ptr header(new ImageHeader(_dinfo->output_width, _dinfo->output_height, cmsType));

	if (_dinfo->saw_JFIF_marker) {
	  switch (_dinfo->density_unit) {
	  case 1:	// pixels per inch (yuck)
	    header->set_resolution(_dinfo->X_density, _dinfo->Y_density);
	    break;

	  case 2:	// pixels per centimetre
	    header->set_resolution(_dinfo->X_density * 2.54, _dinfo->Y_density * 2.54);
	    break;

	  default:
	    std::cerr << "** Unknown density unit (" << (int)_dinfo->density_unit << ") **" << std::endl;
	  }
	}

	cmsHPROFILE profile = jpeg_read_profile(_dinfo);
	if (profile == NULL)
	  profile = default_profile(cmsType);
	header->set_profile(profile);

	this->_send_image_header(header);
      }

      _read_state = 1;
      break;

    case 1:
      _dinfo->dct_method = JDCT_FLOAT; 
      jpeg_start_decompress(_dinfo);

      _read_state = 2;
      break;

    case 2:
      if (_dinfo->output_scanline < _dinfo->output_height) {
	unsigned int y = _dinfo->output_scanline;
	JSAMPROW jpeg_row[1];
	jpeg_row[0] = (JSAMPROW)malloc(_dinfo->output_width * _dinfo->output_components * sizeof(JSAMPROW));
	jpeg_read_scanlines(_dinfo, jpeg_row, 1);
	ImageRow::ptr row(new ImageRow(y, jpeg_row[0]));
	this->_send_image_row(row);
      } else {
	_read_state = 3;
      }

      break;

    case 3:
      jpeg_finish_decompress(_dinfo);
      jpeg_istream_src_free(_dinfo);
      jpeg_destroy_decompress(_dinfo);
      free(_dinfo);
      _dinfo = NULL;
      this->_send_image_end();

      _read_state = 99;

      break;

    default:
      break;
    }
    this->_unlock_reader();
  }

}
