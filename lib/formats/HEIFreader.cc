/*
	Copyright 2019 Ian Tester

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
#include <iostream>
#include <iomanip>

#include <libheif/heif_cxx.h>

namespace fs = boost::filesystem;

namespace PhotoFinish {

  HEIFreader::HEIFreader(const fs::path filepath) :
    ImageReader(filepath)
  {}

  Image::ptr HEIFreader::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    heif::Context ctx;
    ctx.read_from_file(_filepath.native());
    auto image_handle = ctx.get_primary_image_handle();

    CMS::Format format;
    format.set_8bit();
    format.set_colour_model(CMS::ColourModel::RGB);
    format.set_extra_channels(image_handle.has_alpha_channel());

    auto image = std::make_shared<Image>(image_handle.get_width(),
					 image_handle.get_height(),
					 format);

    auto heif_image = image_handle.decode_image(heif_colorspace_RGB,
						image_handle.has_alpha_channel() ? heif_chroma_interleaved_RGBA : heif_chroma_interleaved_RGB);


    return image;
  }

}; // namespace PhotoFinish
