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
#include "Exception.hh"

#include <libheif/heif_cxx.h>

namespace PhotoFinish {

  HEIFwriter::HEIFwriter(const fs::path filepath) :
    ImageWriter(filepath)
  {}

  CMS::Format HEIFwriter::preferred_format(CMS::Format format) {
    if ((format.colour_model() != CMS::ColourModel::RGB)
	&& (format.colour_model() != CMS::ColourModel::Greyscale))
      format.set_colour_model(CMS::ColourModel::RGB);

    format.set_planar();

    format.set_premult_alpha();

    if (format.is_32bit())
      format.set_16bit();

    return format;
  }

  struct heif_plane {
    uint8_t *data;
    int stride;
  };

  void HEIFwriter::write(Image::ptr img, Destination::ptr dest, bool can_free) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    heif::Image heif_image;
    uint32_t num_planes = img->format().channels() + img->format().extra_channels();
    heif_plane planes[3];

    switch (img->format().colour_model()) {
    case CMS::ColourModel::Greyscale:
      if (img->format().bytes_per_channel() > 1)
	throw LibraryError("libheif", "Don't support greyscale > 8bpc");

      heif_image.create(img->width(),
			img->height(),
			heif_colorspace_monochrome,
			heif_chroma_monochrome);
      
      heif_image.add_plane(heif_channel_Y, img->width(), img->height(), 8);
      planes[0].data = heif_image.get_plane(heif_channel_Y, &(planes[0].stride));
      if (img->format().extra_channels()) {
	heif_image.add_plane(heif_channel_Alpha, img->width(), img->height(), 8);
	planes[1].data = heif_image.get_plane(heif_channel_Alpha, &(planes[1].stride));
      }
      break;

    case CMS::ColourModel::RGB:
      heif_chroma chroma;
      int depth;

      if (img->format().is_8bit()) {
	depth = 8;
	if (img->format().extra_channels())
	  chroma = heif_chroma_interleaved_RGBA;
	else
	  chroma = heif_chroma_interleaved_RGB;

      } else {
	depth = 16;
	if (img->format().extra_channels())
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	  chroma = heif_chroma_interleaved_RRGGBBAA_LE;
	else
	  chroma = heif_chroma_interleaved_RRGGBB_LE;
#else
	  chroma = heif_chroma_interleaved_RRGGBBAA_BE;
	else
	  chroma = heif_chroma_interleaved_RRGGBB_BE;
#endif
      }

      heif_image.create(img->width(),
			img->height(),
			heif_colorspace_RGB,
			chroma);

      heif_image.add_plane(heif_channel_R, img->width(), img->height(), depth);
      planes[0].data = heif_image.get_plane(heif_channel_R, &(planes[0].stride));
      heif_image.add_plane(heif_channel_G, img->width(), img->height(), depth);
      planes[1].data = heif_image.get_plane(heif_channel_G, &(planes[1].stride));
      heif_image.add_plane(heif_channel_B, img->width(), img->height(), depth);
      planes[2].data = heif_image.get_plane(heif_channel_B, &(planes[2].stride));
      if (img->format().extra_channels()) {
	heif_image.add_plane(heif_channel_Alpha, img->width(), img->height(), depth);
	planes[3].data = heif_image.get_plane(heif_channel_Alpha, &(planes[3].stride));
      }

      break;

    case CMS::ColourModel::YCbCr:
      if (img->format().bytes_per_channel() > 1)
	throw LibraryError("libheif", "Don't support YCbCr > 8bpc");
      if (img->format().extra_channels())
	throw LibraryError("libheif", "Don't support YCbCr with alpha");

      heif_image.create(img->width(),
			img->height(),
			heif_colorspace_YCbCr,
			heif_chroma_444);

      heif_image.add_plane(heif_channel_Y, img->width(), img->height(), depth);
      planes[0].data = heif_image.get_plane(heif_channel_Y, &(planes[0].stride));
      heif_image.add_plane(heif_channel_Cb, img->width(), img->height(), depth);
      planes[1].data = heif_image.get_plane(heif_channel_Cb, &(planes[1].stride));
      heif_image.add_plane(heif_channel_Cr, img->width(), img->height(), depth);
      planes[2].data = heif_image.get_plane(heif_channel_Cr, &(planes[2].stride));

      break;

    default:
      throw LibraryError("libheif", "Unhandled colour model.");
    }

    /*
    if (img->format().is_planar()) {
      size_t plane_size = img->width() * img->format().bytes_per_channel();

      for (uint32_t y = 0; y < img->height(); y++) {
	for (uint32_t c = 0; c < num_planes; c++)
	  memcpy(planes[c].data + (y * planes[c].stride),
		 img->row(y)->data(0, c),
		 plane_size);
      }
    } else {
      size_t pixel_size = img->format().bytes_per_pixel();
      for (uint32_t y = 0; y < img->height(); y++) {
	for (uint32_t x = 0; x < img->width(); x++) {
	  for (uint32_t c = 0; c < num_planes; c++)
	    memcpy(planes[c].data + (y * planes[c].stride) + (x * pixel_size),
		   img->row(y)->data(x, c),
		   pixel_size);
	}
      }
    }
    */

    heif::Context ctx;
    heif::Encoder encoder(heif_compression_HEVC);

    if (dest->heif().defined()) {
      encoder.set_lossy_quality(dest->heif().lossy_quality());
      encoder.set_lossless(dest->heif().lossless());
    }

    auto img_handle = ctx.encode_image(heif_image, encoder);
    ctx.set_primary_image(img_handle);
    // TODO: EXIF/XMP
    // TODO: ICC?

    ctx.write_to_file(_filepath.native());
  }

}; // namespace PhotoFinish
