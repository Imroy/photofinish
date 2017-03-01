/*
	Copyright 2014 Ian Tester

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
#include <queue>
#include <list>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <string.h>
#include <stdio.h>
#include <jpeglib.h>
#include <omp.h>
#include "ImageFile.hh"
#include "Image.hh"
#include "JPEG.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  void jpeg_error_exit(j_common_ptr cinfo) {
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, buffer);
    jpeg_destroy(cinfo);
    throw LibraryError("libjpeg", buffer);
  }

  JPEGwriter::JPEGwriter(const fs::path filepath) :
    ImageWriter(filepath)
  {}

  CMS::Format JPEGwriter::preferred_format(CMS::Format format) {
    if ((format.colour_model() != CMS::ColourModel::Greyscale)
	&& (format.colour_model() != CMS::ColourModel::CMYK)) {
      format.set_colour_model(CMS::ColourModel::RGB);
    }

    format.set_packed();

    format.set_extra_channels(0);
    format.set_premult_alpha();

    format.set_8bit();

    return format;
  }

  void JPEGwriter::write(std::ostream& os, Image::ptr img, Destination::ptr dest, bool can_free) {
    jpeg_compress_struct *cinfo = (jpeg_compress_struct*)malloc(sizeof(jpeg_compress_struct));
    jpeg_create_compress(cinfo);
    jpeg_error_mgr jerr;
    cinfo->err = jpeg_std_error(&jerr);
    jerr.error_exit = jpeg_error_exit;

    jpeg_ostream_dest(cinfo, &os);

    cinfo->image_width = img->width();
    cinfo->image_height = img->height();

    CMS::Format format = img->format();
    if (format.bytes_per_channel() != 1)
      throw cmsTypeError("Not 8-bit", format);

    switch (format.colour_model()) {
    case CMS::ColourModel::Greyscale:
      cinfo->in_color_space = JCS_GRAYSCALE;
      break;

    case CMS::ColourModel::RGB:
      cinfo->in_color_space = JCS_RGB;
      break;

    case CMS::ColourModel::YCbCr:
      cinfo->in_color_space = JCS_YCbCr;
      break;

    case CMS::ColourModel::CMYK:
      cinfo->in_color_space = JCS_CMYK;
      break;

    default:
      break;
    }
    cinfo->input_components = format.channels();

    jpeg_set_defaults(cinfo);
    {
      // Default values
      int quality = 95;
      int sample_h = 2, sample_v = 2;
      bool progressive = true;

      if (dest->jpeg().defined()) {
	const D_JPEG jpeg = dest->jpeg();
	if (jpeg.quality().defined())
	  quality = jpeg.quality();
	if (jpeg.progressive().defined())
	  progressive = jpeg.progressive();
	if (jpeg.sample().defined() && (format.colour_model() == CMS::ColourModel::RGB)) {
	  sample_h = jpeg.sample()->first;
	  sample_v = jpeg.sample()->second;
	}
      }

      std::cerr << "\tJPEG quality of " << quality << "." << std::endl;
      jpeg_set_quality(cinfo, quality, TRUE);

      if (progressive) {
	std::cerr << "\tProgressive JPEG." << std::endl;
	switch (format.colour_model()) {
	case CMS::ColourModel::Greyscale:
	  jpegfile_scan_greyscale(cinfo);
	  break;

	case CMS::ColourModel::RGB:
	  jpegfile_scan_RGB(cinfo);
	  break;

	default:
	  break;
	}
      }

      if (format.colour_model() != CMS::ColourModel::Greyscale) {
	std::cerr << "\tJPEG chroma sub-sampling of " << sample_h << "×" << sample_v << "." << std::endl;
	cinfo->comp_info[0].h_samp_factor = sample_h;
	cinfo->comp_info[0].v_samp_factor = sample_v;
      }
    }

    cinfo->density_unit = 1;	// PPI
    cinfo->X_density = round(img->xres());
    cinfo->Y_density = round(img->yres());

    cinfo->dct_method = JDCT_FLOAT;

    jpeg_start_compress(cinfo, TRUE);

    if (img->has_profile()) {
      void *profile_data;
      unsigned int profile_len;
      img->profile()->save_to_mem(profile_data, profile_len);

      if (profile_len > 0) {
	if (profile_len > 255 * 65519)
	  std::cerr << "** Profile is too big to fit in APP2 markers! **" << std::endl;
	else {
	  jpeg_write_profile(cinfo, (unsigned char*)profile_data, profile_len);
	  free(profile_data);
	}
      }
    }

    JSAMPROW jpeg_row[1];
    std::cerr << "\tWriting " << img->width() << "×" << img->height() << " JPEG image..." << std::endl;
    while (cinfo->next_scanline < cinfo->image_height) {
      unsigned int y = cinfo->next_scanline;
      jpeg_row[0] = img->row<unsigned char>(y);
      jpeg_write_scanlines(cinfo, jpeg_row, 1);

      if (can_free)
	img->free_row(y);

      std::cerr << "\r\tWritten " << y + 1 << " of " << img->height() << " rows";
    }
    std::cerr << "\r\tWritten " << img->height() << " of " << img->height() << " rows." << std::endl;

    jpeg_finish_compress(cinfo);
    jpeg_ostream_dest_free(cinfo);
    jpeg_destroy_compress(cinfo);
    free(cinfo);
  }

  void JPEGwriter::write(Image::ptr img, Destination::ptr dest, bool can_free) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ofstream ofs(_filepath, std::ios_base::out);
    if (ofs.fail())
      throw FileOpenError(_filepath.native());

    write(ofs, img, dest, can_free);
    ofs.close();
    _is_open = false;

    std::cerr << "\tEmbedding tags..." << std::endl;
    embed_tags(img);

    std::cerr << "Done." << std::endl;
  }

}
