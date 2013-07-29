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
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>
#include "ImageFile.hh"
#include "Image.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  PNGreader::PNGreader(const fs::path filepath) :
    ImageReader(filepath),
    _png(NULL), _info(NULL)
  {}

  struct pngfile_cb_pack {
    Destination::ptr destination;
    Image::ptr image;
  };

  //! Called by libPNG when the iHDR chunk has been read with the main "header" information
  void png_info_cb(png_structp png, png_infop info) {
    png_set_gamma(png, 1.0, 1.0);
    png_set_alpha_mode(png, PNG_ALPHA_PNG, 1.0);
    png_set_packing(png);
    png_set_swap(png);
    png_read_update_info(png, info);

    pngfile_cb_pack *pack = (pngfile_cb_pack*)png_get_progressive_ptr(png);

    unsigned int width, height;
    int bit_depth, colour_type;
    png_get_IHDR(png, info, &width, &height, &bit_depth, &colour_type, NULL, NULL, NULL);
    std::cerr << "\t" << width << "×" << height << ", " << bit_depth << " bpp, type " << colour_type << "." << std::endl;

    pack->destination->set_depth(bit_depth);

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

    switch (colour_type) {
    case PNG_COLOR_TYPE_GRAY_ALPHA:
      format.set_extra_channels(1);
    case PNG_COLOR_TYPE_GRAY:
      format.set_colour_model(CMS::ColourModel::Greyscale);
      break;

    case PNG_COLOR_TYPE_RGB_ALPHA:
      format.set_extra_channels(1);
    case PNG_COLOR_TYPE_RGB:
      format.set_colour_model(CMS::ColourModel::RGB);
      break;

    default:
      std::cerr << "** unsupported PNG colour type " << colour_type << " **" << std::endl;
      exit(1);
    }
    auto img = std::make_shared<Image>(width, height, format);
    pack->image = img;

    {
      unsigned int xres, yres;
      int unit_type;
      if (png_get_pHYs(png, info, &xres, &yres, &unit_type)) {
	switch (unit_type) {
	case PNG_RESOLUTION_METER:
	  img->set_resolution(xres * 0.0254, yres * 0.0254);
	  break;

	case PNG_RESOLUTION_UNKNOWN:
	  break;

	default:
	  std::cerr << "** unknown unit type " << unit_type << " **" << std::endl;
	}
	if (img->xres().defined() && img->yres().defined())
	  std::cerr << "\tImage has resolution of " << img->xres() << "×" << img->yres() << " PPI." << std::endl;
      }
    }

    if (png_get_valid(png, info, PNG_INFO_iCCP)) {
      std::cerr << "\tImage has iCCP chunk." << std::endl;
      char *profile_name;
      int compression_type;
      png_bytep profile_data;
      unsigned int profile_len;
      if (png_get_iCCP(png, info, &profile_name, &compression_type, &profile_data, &profile_len) == PNG_INFO_iCCP) {
	std::cerr << "\tLoading ICC profile \"" << profile_name << "\" from file..." << std::endl;
	CMS::Profile::ptr profile = std::make_shared<CMS::Profile>(profile_data, profile_len);
	void *data_copy = malloc(profile_len);
	memcpy(data_copy, profile_data, profile_len);
	pack->destination->set_profile(profile_name, data_copy, profile_len);
	img->set_profile(profile);
      }
    }
  }

  //! Called by libPNG when a row of image data has been read
  void png_row_cb(png_structp png, png_bytep row_data, png_uint_32 row_num, int pass) {
    pngfile_cb_pack *pack = (pngfile_cb_pack*)png_get_progressive_ptr(png);
    pack->image->check_rowdata_alloc(row_num);
    memcpy(pack->image->row(row_num), row_data, pack->image->row_size());
    std::cerr << "\r\tRead " << (row_num + 1) << " of " << pack->image->height() << " rows";
  }

  //! Called by libPNG when the image data has finished
  void png_end_cb(png_structp png, png_infop info) {
    //    pngfile_cb_pack *pack = (pngfile_cb_pack*)png_get_progressive_ptr(png);
  }


  Image::ptr PNGreader::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream ifs(_filepath, std::ios_base::in);
    if (ifs.fail())
      throw FileOpenError(_filepath.native());

    {
      unsigned char header[8];
      ifs.read((char*)header, 8);
      if (png_sig_cmp(header, 0, 8))
	throw FileContentError(_filepath.string(), "is not a PNG file");
      ifs.seekg(0, std::ios_base::beg);
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
      ifs.close();
      throw LibraryError("libpng", "Something went wrong reading the PNG");
    }

    pngfile_cb_pack pack;
    pack.destination = dest;

    std::cerr << "\tReading PNG image..." << std::endl;
    png_set_progressive_read_fn(_png, (void *)&pack, png_info_cb, png_row_cb, png_end_cb);
    png_byte buffer[1048576];
    size_t length;
    do {
      ifs.read((char*)buffer, 1048576);
      length = ifs.gcount();
      png_process_data(_png, _info, buffer, length);
    } while (length > 0);
    std::cerr << "\r\tRead " << pack.image->height() << " of " << pack.image->height() << " rows." << std::endl;

    png_destroy_read_struct(&_png, &_info, NULL);
    ifs.close();
    _is_open = false;

    std::cerr << "\tExtracting tags..." << std::endl;
    extract_tags(pack.image);

    std::cerr << "Done." << std::endl;
    return pack.image;
  }

}
