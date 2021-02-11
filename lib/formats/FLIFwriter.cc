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
#include <iostream>
#include "ImageFile.hh"

namespace PhotoFinish {

  FLIFwriter::FLIFwriter(const fs::path filepath) :
    ImageWriter(filepath)
  {}

  CMS::Format FLIFwriter::preferred_format(CMS::Format format) {
    if (format.colour_model() != CMS::ColourModel::Greyscale) {
      format.set_colour_model(CMS::ColourModel::RGB);
    }

    format.set_chunky();

    format.set_premult_alpha();

    if (!format.is_8bit() && !format.is_16bit())
      format.set_16bit();

    return format;
  }

  void FLIFwriter::write(Image::ptr img, Destination::ptr dest, bool can_free) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    CMS::Format format = img->format();
    if (format.bytes_per_channel() > 2)
      throw cmsTypeError("Too deep", format);

    FLIF_ENCODER *encoder = flif_create_encoder();

    if (dest->flif().defined()) {
      D_FLIF d = dest->flif();

      flif_encoder_set_interlaced(encoder, d.interlaced());
      flif_encoder_set_learn_repeat(encoder, d.learn_repeat());
      flif_encoder_set_divisor(encoder, d.divisor());
      flif_encoder_set_min_size(encoder, d.min_size());
      flif_encoder_set_split_threshold(encoder, d.split_threshold());
      flif_encoder_set_alpha_zero(encoder, d.alpha_zero());
      flif_encoder_set_chance_cutoff(encoder, d.chance_cutoff());
      flif_encoder_set_chance_alpha(encoder, d.chance_alpha());
      flif_encoder_set_crc_check(encoder, d.crc());
      flif_encoder_set_channel_compact(encoder, d.channel_compact());
      flif_encoder_set_ycocg(encoder, d.ycocg());
      flif_encoder_set_lossy(encoder, d.loss());
    }

    // TODO: Add ICC profile
    // TODO: Add EXIF/IPTC/XMP metadata

    flif_encoder_encode_file(encoder, _filepath.c_str());
    flif_destroy_encoder(encoder);
  }

}; // namespace PhotoFinish
