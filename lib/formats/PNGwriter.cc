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

  PNGwriter::PNGwriter(std::ostream* os, Destination::ptr dest) :
    ImageWriter(os, dest),
    _png(NULL), _info(NULL),
    _next_y(0)
  {
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
      throw LibraryError("libpng", "Something went wrong writing the PNG");
    }

    png_set_write_fn(_png, _os, png_write_ostream_cb, png_flush_ostream_cb);
  }

  void PNGwriter::receive_image_header(ImageHeader::ptr header) {
    int png_colour_type;
    switch (T_COLORSPACE(header->cmsType())) {
    case PT_GRAY:
      png_colour_type = PNG_COLOR_TYPE_GRAY;
      break;

    case PT_RGB:
      png_colour_type = PNG_COLOR_TYPE_RGB;
      break;

    default:
      break;
    }

    int depth = T_BYTES(header->cmsType()) * 8;

    png_set_IHDR(_png, _info,
		 header->width(), header->height(), depth, png_colour_type,
		 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_filter(_png, 0, PNG_ALL_FILTERS);
    png_set_compression_level(_png, Z_BEST_COMPRESSION);

    if (header->xres().defined() && header->yres().defined()) {
      unsigned int xres = round(header->xres() / 0.0254);
      unsigned int yres = round(header->yres() / 0.0254);
      png_set_pHYs(_png, _info, xres, yres, PNG_RESOLUTION_METER);
    }

    cmsUInt32Number intent = INTENT_PERCEPTUAL; // Default value
    if (_dest->intent().defined())
      intent = _dest->intent();
    this->get_and_embed_profile(_dest, header->cmsType(), intent);

    {
      time_t t = time(NULL);
      if (t > 0) {
	png_time ptime;
	png_convert_from_time_t(&ptime, t);
	std::cerr << "\tAdding time chunk." << std::endl;
	png_set_tIME(_png, _info, &ptime);
      }
    }

    if (depth > 8)
      png_set_swap(_png);

    png_write_info(_png, _info);
  }

  void PNGwriter::mark_sGrey(cmsUInt32Number intent) const {
    png_set_sRGB_gAMA_and_cHRM(_png, _info, intent);
  }

  void PNGwriter::mark_sRGB(cmsUInt32Number intent) const {
    png_set_sRGB_gAMA_and_cHRM(_png, _info, intent);
  }

  void PNGwriter::embed_icc(std::string name, unsigned char *data, unsigned int len) const {
    png_set_iCCP(_png, _info, name.c_str(), 0, data, len);
  }

  void PNGwriter::do_work(void) {
    this->_lock_sink_queue();
    ImageRow::ptr row;
    for (_sink_rowqueue_type::iterator rqi = _sink_rowqueue.begin(); rqi != _sink_rowqueue.end(); rqi++)
      if ((*rqi)->y() == _next_y) {
	row = *rqi;
	_sink_rowqueue.erase(rqi);
	break;
      }
    this->_unlock_sink_queue();

    if (row) {
      png_write_row(_png, (png_const_bytep)row->data());
      _next_y++;
    }
  }

  void PNGwriter::receive_image_end(void) {
    png_write_end(_png, _info);
    png_destroy_write_struct(&_png, &_info);
  }

}
