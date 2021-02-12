/*
	Copyright 2021 Ian Tester

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
#include "ImageFile.hh"
#include "Image.hh"
#include <jxl/encode_cxx.h>

namespace fs = boost::filesystem;

namespace PhotoFinish {

  JXLwriter::JXLwriter(const fs::path filepath) :
    ImageWriter(filepath)
  {}

  JxlPixelFormat JXLwriter::pixelformat(const CMS::Format& format) {
    JxlPixelFormat pixelformat;

    pixelformat.num_channels = format.total_channels();

    switch (format.bytes_per_channel()) {
    case 1:
      pixelformat.data_type = JXL_TYPE_UINT8;
      break;

    case 2:
      pixelformat.data_type = JXL_TYPE_UINT16;
      break;

    case 4:
      if (format.is_fp())
	pixelformat.data_type = JXL_TYPE_FLOAT;
      else
	pixelformat.data_type = JXL_TYPE_UINT32;
    }

    pixelformat.align = 0;

    return pixelformat;
  }

  CMS::Format JXLwriter::preferred_format(CMS::Format format) {
    if (format.is_half() || format.is_double())
      format.set_float();

    if ((format.colour_model() != CMS::ColourModel::Greyscale)
	&& (format.colour_model() != CMS::ColourModel::RGB))
      format.set_colour_model(CMS::ColourModel::RGB);

    return format;
  }

  void JXLwriter::write(Image::ptr img, Destination::ptr dest, bool can_free) {
  }

}; // namespace PhotoFinish
