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
#include <errno.h>
#include <png.h>
#include <zlib.h>
#include <time.h>
#include <omp.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>
#include "ImageFile.hh"
#include "Image.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  PNGwriter::PNGwriter(const fs::path filepath) :
    ImageWriter(filepath),
    _png(NULL), _info(NULL)
  {}

  CMS::Format PNGwriter::preferred_format(CMS::Format format) {
    if (format.colour_model() != CMS::ColourModel::Greyscale) {
      format.set_colour_model(CMS::ColourModel::RGB);
    }

    if (!format.is_8bit())
      format.set_16bit();

    format.set_packed();

    format.set_premult_alpha();

    format.unset_swapfirst();
    format.unset_endianswap();

    return format;
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

  void PNGwriter::write(Image::ptr img, Destination::ptr dest, bool can_free) {
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

    CMS::Format format = img->format();
    int png_colour_type;
    switch (format.colour_model()) {
    case CMS::ColourModel::RGB:
      png_colour_type |= PNG_COLOR_MASK_COLOR;
      break;

    case CMS::ColourModel::Greyscale:
      break;

    default:
      throw cmsTypeError("Not RGB or greyscale", format);
    }

    if (format.extra_channels())
      png_colour_type |= PNG_COLOR_MASK_ALPHA;

    int depth = format.bytes_per_channel();
    if (depth > 2)
      throw cmsTypeError("Not 8 or 16-bit", format);

    std::cerr << "\tWriting header for " << img->width() << "×" << img->height()
	      << " " << (depth << 3) << "-bit " << (format.channels() == 1 ? "greyscale" : "RGB")
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

    if (img->has_profile()) {
      CMS::Intent intent = CMS::Intent::Perceptual;        // Default value
      if (dest->intent().defined())
	intent = dest->intent();

      std::string profile_name = img->profile()->description("en", "");
      if ((profile_name.length() > 0) &&
	  (boost::iequals(profile_name, "sGrey built-in") ||
	   boost::iequals(profile_name, "sRGB built-in")))
	png_set_sRGB_gAMA_and_cHRM(_png, _info, (int)intent);
      else {
	unsigned char *profile_data;
	unsigned int profile_len;
	img->profile()->save_to_mem(profile_data, profile_len);
	if (profile_len > 0) {
	  std::cerr << "\tEmbedding profile \"" << profile_name << "\" (" << profile_len << " bytes)." << std::endl;
#if PNG_LIBPNG_VER < 10500
	  png_set_iCCP(_png, _info, (png_charp)profile_name.c_str(), 0, (png_charp)profile_data, profile_len);
#else
	  png_set_iCCP(_png, _info, profile_name.c_str(), 0, (png_const_bytep)profile_data, profile_len);
#endif
	}
      }
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

    if (format.is_vanilla())
      png_set_invert_mono(_png);

    if (depth > 1)
      png_set_swap(_png);

    std::cerr << "\tWriting image..." << std::endl;
    for (unsigned int y = 0; y < img->height(); y++) {
      png_write_row(_png, img->row<unsigned char>(y));

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
