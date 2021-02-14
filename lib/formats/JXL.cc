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

  void getformats(JxlBasicInfo info, JxlPixelFormat& pixelformat, CMS::Format& cmsformat) {
    switch (info.num_color_channels) {
    case 1:
      cmsformat.set_colour_model(CMS::ColourModel::Greyscale);
      break;

    case 3:
      cmsformat.set_colour_model(CMS::ColourModel::RGB);
      break;

    default:
      break;
    }

    pixelformat.num_channels = info.num_color_channels + info.num_extra_channels;
    pixelformat.endianness = JXL_NATIVE_ENDIAN;
    pixelformat.align = 0;

    cmsformat.set_channels(info.num_color_channels);
    cmsformat.set_extra_channels(info.num_extra_channels);
    cmsformat.set_premult_alpha(info.alpha_premultiplied);

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
      if (info.exponent_bits_per_sample > 0) {
	pixelformat.data_type = JXL_TYPE_FLOAT;
	cmsformat.set_fp();
      } else
	pixelformat.data_type = JXL_TYPE_UINT32;
      cmsformat.set_32bit();
      break;

    default:
      throw LibraryError("libjxl", "Unhandled bits_per_sample value");
    }

  }

  void format_info(const CMS::Format& format, JxlPixelFormat& pixel_format, JxlBasicInfo& info, JxlColorEncoding& encoding) {
    pixel_format.num_channels = format.channels() + format.extra_channels();

    info.exponent_bits_per_sample = 0;
    switch (format.bytes_per_channel()) {
    case 1:
      info.bits_per_sample = 8;
      pixel_format.data_type = JXL_TYPE_UINT8;
      break;

    case 2:
      info.bits_per_sample = 16;
      pixel_format.data_type = JXL_TYPE_UINT16;
      break;

    case 4:
      info.bits_per_sample = 32;
      if (format.is_fp()) {
	info.exponent_bits_per_sample = 11;
	pixel_format.data_type = JXL_TYPE_FLOAT;
      } else
	pixel_format.data_type = JXL_TYPE_UINT32;
      break;

    default:
      break;
    };

    if (format.extra_channels() > 0) {
      info.alpha_bits = info.bits_per_sample;
      info.alpha_exponent_bits = info.exponent_bits_per_sample;
    } else
      info.alpha_bits = info.alpha_exponent_bits = 0;

    switch (format.colour_model()) {
    case CMS::ColourModel::Greyscale:
      encoding.color_space = JXL_COLOR_SPACE_GRAY;
      break;

    case CMS::ColourModel::RGB:
      encoding.color_space = JXL_COLOR_SPACE_RGB;
      break;

    default:
      encoding.color_space = JXL_COLOR_SPACE_UNKNOWN;
      break;
    };
  }

}; // namespace PhotoFinish
