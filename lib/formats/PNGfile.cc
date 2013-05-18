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

  PNGfile::PNGfile(const fs::path filepath) :
    ImageFile(filepath),
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

    cmsUInt32Number type = BYTES_SH(bit_depth >> 3);
    switch (colour_type) {
    case PNG_COLOR_TYPE_GRAY:
      type |= COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1);
      break;

    case PNG_COLOR_TYPE_RGB:
      type |= COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3);
      break;

    default:
      std::cerr << "** unsupported PNG colour type " << colour_type << " **" << std::endl;
      exit(1);
    }
    auto img = std::make_shared<Image>(width, height, type);
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
	cmsOpenProfileFromMem(profile_data, profile_len);
	void *data_copy = malloc(profile_len);
	memcpy(data_copy, profile_data, profile_len);
	pack->destination->set_profile(profile_name, data_copy, profile_len);
	img->set_profile(pack->destination->get_profile(type));
      }
    }
  }

  //! Called by libPNG when a row of image data has been read
  void png_row_cb(png_structp png, png_bytep row_data, png_uint_32 row_num, int pass) {
    pngfile_cb_pack *pack = (pngfile_cb_pack*)png_get_progressive_ptr(png);
    memcpy(pack->image->row(row_num), row_data, pack->image->row_size());
    std::cerr << "\r\tRead " << (row_num + 1) << " of " << pack->image->height() << " rows";
  }

  //! Called by libPNG when the image data has finished
  void png_end_cb(png_structp png, png_infop info) {
    //    pngfile_cb_pack *pack = (pngfile_cb_pack*)png_get_progressive_ptr(png);
  }


  Image::ptr PNGfile::read(Destination::ptr dest) {
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

  cmsUInt32Number PNGfile::preferred_type(cmsUInt32Number type) {
    if (T_COLORSPACE(type) != PT_GRAY) {
      type &= COLORSPACE_MASK;
      type |= COLORSPACE_SH(PT_RGB);
      type &= CHANNELS_MASK;
      type |= CHANNELS_SH(3);
    }

    type &= EXTRA_MASK;

    type &= FLOAT_MASK;
    if ((T_BYTES(type) == 0) || (T_BYTES(type) > 2)) {
      type &= BYTES_MASK;
      type |= BYTES_SH(2);
    } else {
      type &= BYTES_MASK;
      type |= BYTES_SH(1);
    }

    type &= PLANAR_MASK;
    type &= SWAPFIRST_MASK;
    type &= DOSWAP_MASK;

    return type;
  }

  //! libPNG callback for writing to an ostream
  void png_write_ostream_cb(png_structp png, png_bytep buffer, png_size_t length) {
    std::ostream *os = (std::ostream*)png_get_io_ptr(png);
    os->write((char*)buffer, length);
  }

  //! libPNG callback for flushing an ostream
  void png_flush_ostream_cb(png_structp png) {
    std::ostream *os = (std::ostream*)png_get_io_ptr(png);
    os->flush();
  }

  void PNGfile::write(Image::ptr img, Destination::ptr dest, bool can_free) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ofstream ofs;
    ofs.open(_filepath, std::ios_base::out);

    _png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
					      NULL, NULL, NULL);
    if (!_png)
      throw LibraryError("libpng", "Could not create PNG write structure");

    _info = png_create_info_struct(_png);
    if (!_info) {
      png_destroy_write_struct(&_png, (png_infopp)NULL);
      throw LibraryError("libpng", "Could not create PNG info structure");
    }

    if (setjmp(png_jmpbuf(_png))) {
      png_destroy_write_struct(&_png, &_info);
      ofs.close();
      throw LibraryError("libpng", "Something went wrong writing the PNG");
    }

    png_set_write_fn(_png, &ofs, png_write_ostream_cb, png_flush_ostream_cb);

    cmsUInt32Number type = img->type();
    int png_colour_type;
    switch (T_COLORSPACE(type)) {
    case PT_RGB:
      png_colour_type = PNG_COLOR_TYPE_RGB;
      break;

    case PT_GRAY:
      png_colour_type = PNG_COLOR_TYPE_GRAY;
      break;

    default:
      throw cmsTypeError("Not RGB or greyscale", type);
    }

    int depth = T_BYTES(type);
    if (depth > 2)
      throw cmsTypeError("Not 8 or 16-bit", type);

    std::cerr << "\tWriting header for " << img->width() << "×" << img->height()
	      << " " << (depth << 3) << "-bit " << (T_CHANNELS(type) == 1 ? "greyscale" : "RGB")
	      << " PNG image..." << std::endl;
    png_set_IHDR(_png, _info,
		 img->width(), img->height(), depth << 3, png_colour_type,
		 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_filter(_png, 0, PNG_ALL_FILTERS);
    png_set_compression_level(_png, Z_BEST_COMPRESSION);

    if (img->xres().defined() && img->yres().defined()) {
      unsigned int xres = round(img->xres() / 0.0254);
      unsigned int yres = round(img->yres() / 0.0254);
      png_set_pHYs(_png, _info, xres, yres, PNG_RESOLUTION_METER);
    }

    {
      cmsUInt32Number intent = INTENT_PERCEPTUAL;        // Default value
      if (dest->intent().defined())
	intent = dest->intent();

      char *profile_name = NULL;
      unsigned int profile_name_len;
      if ((profile_name_len = cmsGetProfileInfoASCII(img->profile(), cmsInfoDescription, "en", cmsNoCountry, NULL, 0)) > 0) {
	profile_name = (char*)malloc(profile_name_len);
	cmsGetProfileInfoASCII(img->profile(), cmsInfoDescription, "en", cmsNoCountry, profile_name, profile_name_len);

	if (strncmp(profile_name, "sGrey built-in", 14) == 0)
	  png_set_sRGB_gAMA_and_cHRM(_png, _info, intent);
	else if (strncmp(profile_name, "sRGB built-in", 13) == 0)
	  png_set_sRGB_gAMA_and_cHRM(_png, _info, intent);
      }

      unsigned char *profile_data = NULL;
      unsigned int profile_len = 0;
      cmsSaveProfileToMem(img->profile(), NULL, &profile_len);
      if (profile_len > 0) {
	profile_data = (unsigned char*)malloc(profile_len);
	cmsSaveProfileToMem(img->profile(), profile_data, &profile_len);

	std::cerr << "\tEmbedding profile \"" << profile_name << "\" (" << profile_len << " bytes)." << std::endl;
	png_set_iCCP(_png, _info, profile_name, 0, profile_data, profile_len);
      }
      if (profile_name)
	free(profile_name);
    }

    {
      time_t t = time(NULL);
      if (t > 0) {
	png_time ptime;
	png_convert_from_time_t(&ptime, t);
	std::cerr << "\tAdding time chunk." << std::endl;
	png_set_tIME(_png, _info, &ptime);
      }
    }

    png_write_info(_png, _info);

    if (T_FLAVOR(type))
      png_set_invert_mono(_png);

    if (depth > 1)
      png_set_swap(_png);

    std::cerr << "\tWriting image..." << std::endl;
    for (unsigned int y = 0; y < img->height(); y++) {
      png_write_row(_png, (png_const_bytep)img->row(y));

      if (can_free)
	img->free_row(y);

      std::cerr << "\r\tWritten " << y + 1 << " of " << img->height() << " rows";
    }
    std::cerr << "\r\tWritten " << img->height() << " of " << img->height() << " rows." << std::endl;

    png_write_end(_png, _info);

    png_destroy_write_struct(&_png, &_info);
    ofs.close();
    _is_open = false;

    std::cerr << "\tEmbedding tags..." << std::endl;
    embed_tags(img);

    std::cerr << "Done." << std::endl;
  }

}
