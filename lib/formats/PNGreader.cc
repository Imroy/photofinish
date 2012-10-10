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
#include <errno.h>
#include <png.h>
#include <zlib.h>
#include <time.h>
#include <omp.h>
#include <lcms2.h>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>
#include "ImageFile.hh"
#include "Image.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  void png_info_cb(png_structp png, png_infop info);
  void png_row_cb(png_structp png, png_bytep row_data, png_uint_32 row_num, int pass);
  void png_end_cb(png_structp png, png_infop info);

  PNGreader::PNGreader(std::istream* is) :
    ImageReader(is),
    _png(NULL), _info(NULL),
    _buffer((unsigned char*)malloc(32768))
  {
    {
      unsigned char header[8];
      _is->read((char*)header, 8);
      if (png_sig_cmp(header, 0, 8))
	throw FileContentError("stream", "is not a PNG file");
      _is->seekg(0, std::ios_base::beg);
    }

    _png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					     NULL, NULL, NULL);
    if (!_png)
      throw LibraryError("libpng", "Could not create PNG read structure");

    _info = png_create_info_struct(_png);
    if (!_info) {
      png_destroy_read_struct(&_png, (png_infopp)NULL, (png_infopp)NULL);
      throw LibraryError("libpng", "Could not create PNG info structure");
    }

    if (setjmp(png_jmpbuf(_png))) {
      png_destroy_read_struct(&_png, &_info, NULL);
      throw LibraryError("libpng", "Something went wrong reading the PNG");
    }

    png_set_progressive_read_fn(_png, (void *)this, png_info_cb, png_row_cb, png_end_cb);
  }

  PNGreader::~PNGreader() {
    free(_buffer);
    _buffer = NULL;
  }

  void PNGreader::do_work(void) {
    if (!this->_test_reader_lock())
      return;

    size_t length;
    switch (_read_state) {
    case 0:
      _is->read((char*)_buffer, 32768);
      length = _is->gcount();
      if (length > 0)
	png_process_data(_png, _info, _buffer, length);
      if (_is->eof())
	_read_state = 1;
      break;

    case 1:
      png_destroy_read_struct(&_png, &_info, NULL);

      _read_state = 99;
      break;

    default:
      break;
    }
    this->_unlock_reader();
  }

  //! Called by libPNG when the iHDR chunk has been read with the main "header" information
  void png_info_cb(png_structp png, png_infop info) {
    png_set_gamma(png, 1.0, 1.0);
    png_set_alpha_mode(png, PNG_ALPHA_PNG, 1.0);
    png_set_packing(png);
    png_set_swap(png);
    png_read_update_info(png, info);

    PNGreader *self = (PNGreader*)png_get_progressive_ptr(png);

    unsigned int width, height;
    int bit_depth, colour_type;
    png_get_IHDR(png, info, &width, &height, &bit_depth, &colour_type, NULL, NULL, NULL);

    ImageHeader::ptr header(new ImageHeader(width, height));

    cmsUInt32Number cmsType;
    switch (colour_type) {
    case PNG_COLOR_TYPE_GRAY:
      cmsType = COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(bit_depth >> 3);
      break;

    case PNG_COLOR_TYPE_RGB:
      cmsType = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(bit_depth >> 3);
      break;

    default:
      std::cerr << "** unsupported PNG colour type " << colour_type << " **" << std::endl;
      exit(1);
    }
    header->set_cmsType(cmsType);

    {
      unsigned int xres, yres;
      int unit_type;
      if (png_get_pHYs(png, info, &xres, &yres, &unit_type)) {
	switch (unit_type) {
	case PNG_RESOLUTION_METER:
	  header->set_resolution(xres * 0.0254, yres * 0.0254);
	  break;
	case PNG_RESOLUTION_UNKNOWN:
	  break;
	default:
	  std::cerr << "** unknown unit type " << unit_type << " **" << std::endl;
	}
      }
    }

    cmsHPROFILE profile = NULL;

    if (png_get_valid(png, info, PNG_INFO_iCCP)) {
      std::cerr << "\tImage has iCCP chunk." << std::endl;
      char *profile_name;
      int compression_type;
      png_bytep profile_data;
      unsigned int profile_len;
      if (png_get_iCCP(png, info, &profile_name, &compression_type, &profile_data, &profile_len) == PNG_INFO_iCCP) {
	std::cerr << "\tLoading ICC profile \"" << profile_name << "\" from file..." << std::endl;
	profile = cmsOpenProfileFromMem(profile_data, profile_len);
	header->set_profile(profile);
      }
    }
    if (profile == NULL) {
      profile = default_profile(cmsType);
      header->set_profile(profile);
    }

    self->_send_image_header(header);
  }

  //! Called by libPNG when a row of image data has been read
  void png_row_cb(png_structp png, png_bytep row_data, png_uint_32 row_num, int pass) {
    PNGreader *self = (PNGreader*)png_get_progressive_ptr(png);
    ImageRow::ptr row(new ImageRow(row_num, row_data));
    self->_send_image_row(row);
  }

  //! Called by libPNG when the image data has finished
  void png_end_cb(png_structp png, png_infop info) {
    PNGreader *self = (PNGreader*)png_get_progressive_ptr(png);
    self->_send_image_end();
  }

}
