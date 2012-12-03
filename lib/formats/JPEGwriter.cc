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

  void jpeg_ostream_dest(j_compress_ptr cinfo, std::ostream* os);
  void jpeg_ostream_dest_free(j_compress_ptr cinfo);

  JPEGwriter::JPEGwriter(std::ostream* os, Destination::ptr dest, bool close_on_end) :
    ImageWriter(os, dest, close_on_end),
    _cinfo(NULL)
  {
    _cinfo = (jpeg_compress_struct*)malloc(sizeof(jpeg_compress_struct));
    jpeg_create_compress(_cinfo);
    jpeg_error_mgr *jerr = (jpeg_error_mgr*)malloc(sizeof(jpeg_error_mgr));
    _cinfo->err = jpeg_std_error(jerr);

    jpeg_ostream_dest(_cinfo, _os);
  }

  //! Create a scan "script" for an RGB image
  void jpegfile_scan_RGB(jpeg_compress_struct* cinfo);

  //! Create a scan "script" for a greyscale image
  void jpegfile_scan_greyscale(jpeg_compress_struct* cinfo);

  void JPEGwriter::receive_image_header(ImageHeader::ptr header) {
    ImageWriter::receive_image_header(header);

    _cinfo->image_width = header->width();
    _cinfo->image_height = header->height();

    _cinfo->input_components = T_CHANNELS(header->cmsType());
    switch (T_COLORSPACE(header->cmsType())) {
    case PT_GRAY:
      _cinfo->in_color_space = JCS_GRAYSCALE;
      break;

    case PT_RGB:
      _cinfo->in_color_space = JCS_RGB;
      break;

    default:
      // Oh shit
      break;
    }

    jpeg_set_defaults(_cinfo);
    {
      // Default values
      int quality = 95;
      int sample_h = 2, sample_v = 2;
      bool progressive = true;

      if (_dest->jpeg().defined()) {
	if (_dest->jpeg().quality().defined())
	  quality = _dest->jpeg().quality();
	if (_dest->jpeg().progressive().defined())
	  progressive = _dest->jpeg().progressive();
	if (_dest->jpeg().sample().defined() && (_cinfo->input_components > 1)) {
	  sample_h = _dest->jpeg().sample()->first;
	  sample_v = _dest->jpeg().sample()->second;
	}
      }

      std::cerr << "\tJPEG quality of " << quality << "." << std::endl;
      jpeg_set_quality(_cinfo, quality, TRUE);

      if (progressive) {
	std::cerr << "\tProgressive JPEG." << std::endl;
	if (_cinfo->in_color_space == JCS_GRAYSCALE)
	  jpegfile_scan_greyscale(_cinfo);
	else
	  jpegfile_scan_RGB(_cinfo);
      }

      if (_cinfo->in_color_space == JCS_RGB) {
	std::cerr << "\tJPEG chroma sub-sampling of " << sample_h << "×" << sample_v << "." << std::endl;
	_cinfo->comp_info[0].h_samp_factor = sample_h;
	_cinfo->comp_info[0].v_samp_factor = sample_v;
      }
    }

    _cinfo->density_unit = 1;	// PPI
    _cinfo->X_density = round(header->xres());
    _cinfo->Y_density = round(header->yres());

    _cinfo->dct_method = JDCT_FLOAT;

    jpeg_start_compress(_cinfo, TRUE);

    cmsUInt32Number intent = INTENT_PERCEPTUAL; // Default value
    if (_dest->intent().defined())
      intent = _dest->intent();

    this->get_and_embed_profile(header->profile(), header->cmsType(), intent);
  }

  void JPEGwriter::_write_row(ImageRow::ptr row) {
    JSAMPROW jpeg_row[1];
    jpeg_row[0] = (JSAMPROW)row->data();
    jpeg_write_scanlines(_cinfo, jpeg_row, 1);

  }

  void JPEGwriter::_finish_writing(void) {
    jpeg_finish_compress(_cinfo);
    jpeg_ostream_dest_free(_cinfo);
    jpeg_destroy_compress(_cinfo);
  }

  void JPEGwriter::mark_sGrey(cmsUInt32Number intent) const {
  }

  void JPEGwriter::mark_sRGB(cmsUInt32Number intent) const {
  }

  void jpeg_write_profile(jpeg_compress_struct* cinfo, unsigned char *data, unsigned int size);

  void JPEGwriter::embed_icc(std::string name, unsigned char *data, unsigned int len) const {
    jpeg_write_profile(_cinfo, data, len);
  }

}
