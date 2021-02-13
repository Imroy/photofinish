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

#include "JXL.hh"

namespace PhotoFinish {

  CMS::Format cmsformat(const JxlPixelFormat& pixelformat) {
    CMS::Format format;

    switch (pixelformat.num_channels) {
    case 2:
      format.set_extra_channels(1);
    case 1:
      format.set_channels(1);
      format.set_colour_model(CMS::ColourModel::Greyscale);
      break;

    case 4:
      format.set_extra_channels(1);
    case 3:
      format.set_channels(3);
      format.set_colour_model(CMS::ColourModel::RGB);
      break;

    default:
      break;
    }

    switch (pixelformat.data_type) {
    case JXL_TYPE_FLOAT:
      format.set_float();
      break;

    case JXL_TYPE_UINT8:
      format.set_8bit();
      break;

    case JXL_TYPE_UINT16:
      format.set_16bit();
      break;

    case JXL_TYPE_UINT32:
      format.set_32bit();
      break;

    default:
      break;
    }

    return format;
  }

  void getformats(JxlBasicInfo info, JxlPixelFormat& pixelformat, CMS::Format& cmsformat) {
    switch (info.bits_per_sample) {
    case 8:
      pixelformat.data_type = JXL_TYPE_UINT8;
      cmsformat.set_8bit();
      break;

    case 16:
      pixelformat.data_type = JXL_TYPE_UINT16;
      cmsformat.set_16bit();
      break;

    case 32:
      pixelformat.data_type = JXL_TYPE_UINT32;
      cmsformat.set_32bit();
      break;

    default:
      throw LibraryError("libjxl", "Unhandled bits_per_sample value");
    }
  }

  JxlPixelFormat pixelformat(const CMS::Format& format) {
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

}; // namespace PhotoFinish
