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

namespace fs = boost::filesystem;

namespace PhotoFinish {

  FLIFreader::FLIFreader(const fs::path filepath) :
    ImageReader(filepath)
  {}

  uint32_t flif_read_callback(uint32_t quality, int64_t bytes_read, uint8_t decode_over, void *user_data, void *context) {
    double mult;
    std::string unit = "bytes";
    if (bytes_read > 1099511627776) {
      mult = 1.0 / 1099511627776;
      unit = "TiB";
    } else if (bytes_read > 1073741824) {
      mult = 1.0 / 1073741824;
      unit = "GiB";
    } else if (bytes_read > 1048576) {
      mult = 1.0 / 1048576;
      unit = "MiB";
    } else if (bytes_read > 1024) {
      mult = 1.0 / 1024;
      unit = "kiB";
    }

    std::cerr << "\r\tLoaded " << std::setprecision(2) << std::fixed << (bytes_read * mult) << " " << unit << ", progress=" << (quality * 0.01) << "%  ";
    return quality + 1000;
  }

  Image::ptr FLIFreader::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    FLIF_DECODER *decoder = flif_create_decoder();
    flif_decoder_set_callback(decoder, flif_read_callback, nullptr);
    flif_decoder_set_first_callback_quality(decoder, 1000);
    flif_decoder_decode_file(decoder, _filepath.c_str());
    std::cerr << std::endl;

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
    case 1:
      format.set_colour_model(CMS::ColourModel::Greyscale);
      break;

    case 4:
      format.set_extra_channels(1);
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

    {
      uint8_t *profile_data;
      size_t profile_len;
      if (flif_image_get_metadata(flif_image, "iCCP", &profile_data, &profile_len)) {
	std::cerr << "\tImage has iCCP chunk." << std::endl;
	CMS::Profile::ptr profile = std::make_shared<CMS::Profile>(profile_data, profile_len);
	unsigned char *data_copy = new unsigned char[profile_len];
	memcpy(data_copy, profile_data, profile_len);

	std::string profile_name = profile->description("en", "");
	if (profile_name.length() > 0)
	  dest->set_profile(profile_name, data_copy, profile_len);
	else
	  dest->set_profile("FLIF ICC profile", data_copy, profile_len);
	image->set_profile(profile);

	flif_image_free_metadata(flif_image, profile_data);
      }
    }

    // TODO: Get EXIF/IPTC/XMP metadata
 
#pragma omp parallel for schedule(dynamic, 1)
    for (uint32_t y = 0; y < image->height(); y++) {
      image->check_row_alloc(y);

      // libflif only returns greyscale and RGBA (8 or 16 bpp)
      // Greyscale with alpha and RGB has to be extracted from RGBA
      if ((format.channels() == 1) && (format.extra_channels() == 0)) {
	switch(format.bytes_per_channel()) {
	case 1: flif_image_read_row_GRAY8(flif_image, y, image->row(y)->data<void>(), image->row_size());
	  break;

	case 2: flif_image_read_row_GRAY16(flif_image, y, image->row(y)->data<void>(), image->row_size());
	  break;

	default:
	  break;
	}

      } else if ((format.channels() == 3) && (format.extra_channels() == 1)) {
	switch(format.bytes_per_channel()) {
	case 1: flif_image_read_row_RGBA8(flif_image, y, image->row(y)->data<void>(), image->row_size());
	  break;

	case 2: flif_image_read_row_RGBA16(flif_image, y, image->row(y)->data<void>(), image->row_size());
	  break;

	default:
	  break;
	}

      } else {
	if (format.bytes_per_channel() == 1) {
	  uint8_t temp[image->width() * 4];
	  flif_image_read_row_RGBA8(flif_image, y, temp, sizeof(temp));

	  uint8_t *in = temp, *out = image->row(y)->data<uint8_t>();
	  switch (format.channels()) {
	  case 1: // Greyscale with alpha - grey value is in the 'blue' channel
	    for (uint32_t x = 0; x < image->width(); x++, in += 4, out += 2) {
	      out[0] = in[2];
	      out[1] = in[3];
	    }
	    break;

	  case 3: // RGB - throw out empty alpha channel
	    for (uint32_t x = 0; x < image->width(); x++, in += 4, out += 3) {
	      out[0] = in[0];
	      out[1] = in[1];
	      out[2] = in[2];
	    }
	    break;
	  }

	} else {
	  uint16_t temp[image->width() * 4];
	  flif_image_read_row_RGBA16(flif_image, y, temp, sizeof(temp));

	  uint16_t *in = temp, *out = image->row(y)->data<uint16_t>();
	  switch (format.channels()) {
	  case 1: // Greyscale with alpha - grey value is in the 'blue' channel
	    for (uint32_t x = 0; x < image->width(); x++, in += 4, out += 2) {
	      out[0] = in[2];
	      out[1] = in[3];
	    }
	    break;

	  case 3: // RGB - throw out empty alpha channel
	    for (uint32_t x = 0; x < image->width(); x++, in += 4, out += 3) {
	      out[0] = in[0];
	      out[1] = in[1];
	      out[2] = in[2];
	    }
	    break;

	  }
	}
      }

    }

    flif_destroy_decoder(decoder);

    return image;
  }

}; // namespace PhotoFinish
