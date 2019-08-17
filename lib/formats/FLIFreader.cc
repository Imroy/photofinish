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

namespace fs = boost::filesystem;

namespace PhotoFinish {

  FLIFreader::FLIFreader(const fs::path filepath) :
    ImageReader(filepath)
  {}

  Image::ptr FLIFreader::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    FLIF_DECODER *decoder = flif_create_decoder();
    flif_decoder_decode_file(decoder, _filepath.c_str());

    FLIF_IMAGE *flif_image = flif_decoder_get_image(decoder, 0);

    CMS::Format format;
    switch (flif_image_get_depth(flif_image)) {
    case 8: format.set_8bit();
      break;

    case 16: format.set_16bit();
      break;

    default:
      std::cerr << "** Unknown depth " << flif_image_get_depth(flif_image) << " **" << std::endl;
      throw LibraryError("libflif", "depth");
    }

    switch (flif_image_get_nb_channels(flif_image)) {
    case 2:
      format.set_extra_channels(1);
      format.set_premult_alpha();
    case 1:
      format.set_colour_model(CMS::ColourModel::Greyscale);
      break;

    case 4:
      format.set_extra_channels(1);
      format.set_premult_alpha();
    case 3:
      format.set_colour_model(CMS::ColourModel::RGB);
      break;

    default:
      std::cerr << "** Unknown number of channels " << flif_image_get_nb_channels(flif_image) << " **" << std::endl;
      throw LibraryError("libflif", "channels");
    }

    auto image = std::make_shared<Image>(flif_image_get_width(flif_image),
					 flif_image_get_height(flif_image),
					 format);

    // TODO: Get ICC profile
    // TODO: Get EXIF/IPTC/XMP metadata
 
#pragma omp parallel for schedule(dynamic, 1)
    for (uint32_t y = 0; y < image->height(); y++) {
      image->check_rowdata_alloc(y);

      switch (format.channels()) {
      case 1:
	switch(format.bytes_per_channel()) {
	case 1: flif_image_read_row_GRAY8(flif_image, y, image->row<void>(y), image->row_size());
	  break;
	case 2: flif_image_read_row_GRAY16(flif_image, y, image->row<void>(y), image->row_size());
	  break;
	}
	break;
      case 3:
	switch(format.bytes_per_channel()) {
	case 1: flif_image_read_row_RGBA8(flif_image, y, image->row<void>(y), image->row_size());
	  break;
	case 2: flif_image_read_row_RGBA16(flif_image, y, image->row<void>(y), image->row_size());
	  break;
	}
	break;
      }
    }

    flif_destroy_decoder(decoder);

    return image;
  }

}; // namespace PhotoFinish
