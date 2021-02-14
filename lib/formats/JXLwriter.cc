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

  CMS::Format JXLwriter::preferred_format(CMS::Format format) {
    if (format.is_half() || format.is_double())
      format.set_float();

    if ((format.colour_model() != CMS::ColourModel::Greyscale)
	&& (format.colour_model() != CMS::ColourModel::RGB))
      format.set_colour_model(CMS::ColourModel::RGB);

    if (format.is_half() || format.is_double())
      format.set_float();

    format.unset_swapfirst();
    format.unset_endian16_swap();

    return format;
  }

  void JXLwriter::write(Image::ptr img, Destination::ptr dest, bool can_free) {
  }

}; // namespace PhotoFinish
