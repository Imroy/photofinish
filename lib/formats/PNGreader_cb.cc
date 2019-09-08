/*
	Copyright 2014-2019 Ian Tester

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
#include "PNGreader_cb.hh"

namespace PhotoFinish {

  PNGreader_cb::PNGreader_cb(Destination::ptr d) :
    _destination(d)
  {}

  void PNGreader_cb::info(png_structp png, png_infop info) {
    png_set_gamma(png, 1.0, 1.0);
#ifdef PNG_READ_ALPHA_MODE_SUPPORTED
    png_set_alpha_mode(png, PNG_ALPHA_PNG, 1.0);
#endif
    png_set_packing(png);
    png_set_swap(png);
    png_read_update_info(png, info);

    png_uint_32 width, height;
    int bit_depth, colour_type;
    png_get_IHDR(png, info, &width, &height, &bit_depth, &colour_type, nullptr, nullptr, nullptr);
    std::cerr << "\t" << width << "×" << height << ", " << bit_depth << " bpp, type " << colour_type << "." << std::endl;

    _destination->set_depth(bit_depth);

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
      throw LibraryError("libpng", "depth");
    }

    if (colour_type & PNG_COLOR_MASK_PALETTE) {
      std::cerr << "** unsupported PNG colour type " << colour_type << " **" << std::endl;
      throw LibraryError("libpng", "colour type");
    }

    if (colour_type & PNG_COLOR_MASK_COLOR)
      format.set_colour_model(CMS::ColourModel::RGB);
    else
      format.set_colour_model(CMS::ColourModel::Greyscale);

    if (colour_type & PNG_COLOR_MASK_ALPHA) {
      format.set_extra_channels(1);
      format.set_premult_alpha();
    }

    _image = std::make_shared<Image>(width, height, format);

    {
      png_uint_32 xres, yres;
      int unit_type;
      if (png_get_pHYs(png, info, &xres, &yres, &unit_type)) {
	switch (unit_type) {
	case PNG_RESOLUTION_METER:
	  _image->set_resolution(xres * 0.0254, yres * 0.0254);
	  break;

	case PNG_RESOLUTION_UNKNOWN:
	  break;

	default:
	  std::cerr << "** unknown unit type " << unit_type << " **" << std::endl;
	}
	if (_image->xres().defined() && _image->yres().defined())
	  std::cerr << "\tImage has resolution of " << _image->xres() << "×" << _image->yres() << " PPI." << std::endl;
      }
    }

    if (png_get_valid(png, info, PNG_INFO_iCCP)) {
      std::cerr << "\tImage has iCCP chunk." << std::endl;
      png_charp profile_name;
      int compression_type;
      png_bytep profile_data;
      png_uint_32 profile_len;
#if PNG_LIBPNG_VER < 10500
      if (png_get_iCCP(png, info, &profile_name, &compression_type, (png_charpp)&profile_data, &profile_len) == PNG_INFO_iCCP) {
#else
      if (png_get_iCCP(png, info, &profile_name, &compression_type, &profile_data, &profile_len) == PNG_INFO_iCCP) {
#endif
	std::cerr << "\tLoading ICC profile \"" << profile_name << "\" from file..." << std::endl;
	CMS::Profile::ptr profile = std::make_shared<CMS::Profile>(profile_data, profile_len);
	unsigned char *data_copy = new unsigned char[profile_len];
	memcpy(data_copy, profile_data, profile_len);
	_destination->set_profile(profile_name, data_copy, profile_len);
	_image->set_profile(profile);
      }
    }
  }

  void png_info_cb(png_structp png, png_infop info) {
    PNGreader_cb *cb = (PNGreader_cb*)png_get_progressive_ptr(png);
    cb->info(png, info);
  }

  void PNGreader_cb::row(png_structp png, png_bytep row_data, png_uint_32 row_num, int pass) {
    _image->check_row_alloc(row_num);
    memcpy(_image->row(row_num)->data(), row_data, _image->row_size());
    std::cerr << "\r\tRead " << (row_num + 1) << " of " << _image->height() << " rows";
  }

  void png_row_cb(png_structp png, png_bytep row_data, png_uint_32 row_num, int pass) {
    PNGreader_cb *cb = (PNGreader_cb*)png_get_progressive_ptr(png);
    cb->row(png, row_data, row_num, pass);
  }

  void PNGreader_cb::end(png_structp png, png_infop info) {
    std::cerr << "\r\tRead " << _image->height() << " of " << _image->height() << " rows." << std::endl;
  }

  void png_end_cb(png_structp png, png_infop info) {
    PNGreader_cb *cb = (PNGreader_cb*)png_get_progressive_ptr(png);
    cb->end(png, info);
  }




} // namespace PhotoFinish
