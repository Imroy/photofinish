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
#pragma once

#include <png.h>
#include "Destination.hh"
#include "Image.hh"

namespace PhotoFinish {

  struct PNGreader_cb {
    Destination::ptr _destination;
    Image::ptr _image;

    PNGreader_cb(Destination::ptr d);

    void info(png_structp png, png_infop info);

    void row(png_structp png, png_bytep row_data, png_uint_32 row_num, int pass);

    void end(png_structp png, png_infop info);

  }; // class PNGreader_cb

  //! Called by libPNG when the iHDR chunk has been read with the main "header" information
  void png_info_cb(png_structp png, png_infop info);

  //! Called by libPNG when a row of image data has been read
  void png_row_cb(png_structp png, png_bytep row_data, png_uint_32 row_num, int pass);

  //! Called by libPNG when the image data has finished
  void png_end_cb(png_structp png, png_infop info);

} // namespace PhotoFinish
