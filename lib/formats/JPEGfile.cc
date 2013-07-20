/*
	Copyright 2012 Ian Tester

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
#include <setjmp.h>
#include <omp.h>
#include "ImageFile.hh"
#include "Image.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  JPEGfile::JPEGfile(const fs::path filepath) :
    ImageFile(filepath)
  {}

  //! Set up a "source manager" on the given JPEG decompression structure to read from an istream
  void jpeg_istream_src(j_decompress_ptr dinfo, std::istream* is);

  //! Free the data structures of the istream source manager
  void jpeg_istream_src_free(j_decompress_ptr dinfo);

  //! Read an ICC profile from APP2 markers in a JPEG file
  CMS::Profile::ptr jpeg_read_profile(jpeg_decompress_struct*, Destination::ptr dest);

  Image::ptr JPEGfile::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream ifs(_filepath, std::ios_base::in);
    if (ifs.fail())
      throw FileOpenError(_filepath.native());

    jpeg_decompress_struct *dinfo = (jpeg_decompress_struct*)malloc(sizeof(jpeg_decompress_struct));
    jpeg_create_decompress(dinfo);
    struct jpeg_error_mgr jerr;
    dinfo->err = jpeg_std_error(&jerr);

    jpeg_istream_src(dinfo, &ifs);

    jpeg_save_markers(dinfo, JPEG_APP0 + 2, 0xFFFF);

    jpeg_read_header(dinfo, TRUE);
    dinfo->dct_method = JDCT_FLOAT;

    jpeg_start_decompress(dinfo);

    CMS::Format format;
    format.set_8bit();
    switch (dinfo->jpeg_color_space) {
    case JCS_GRAYSCALE:
      format.set_colour_model(CMS::ColourModel::Greyscale, dinfo->num_components);
      break;

    case JCS_YCbCr:
      dinfo->out_color_space = JCS_RGB;
    case JCS_RGB:
      format.set_colour_model(CMS::ColourModel::RGB, dinfo->num_components);
      break;

    case JCS_YCCK:
      dinfo->out_color_space = JCS_CMYK;
    case JCS_CMYK:
      format.set_colour_model(CMS::ColourModel::CMYK, dinfo->num_components);
      if (dinfo->saw_Adobe_marker)
	format.set_vanilla();
      break;

    default:
      std::cerr << "** unsupported JPEG colour space " << dinfo->jpeg_color_space << " **" << std::endl;
      exit(1);
    }

    auto img = std::make_shared<Image>(dinfo->output_width, dinfo->output_height, format);
    dest->set_depth(8);

    if (dinfo->saw_JFIF_marker) {
      switch (dinfo->density_unit) {
      case 1:	// pixels per inch (yuck)
	img->set_resolution(dinfo->X_density, dinfo->Y_density);
	break;

      case 2:	// pixels per centimetre
	img->set_resolution(dinfo->X_density * 2.54, dinfo->Y_density * 2.54);
	break;

      default:
	std::cerr << "** Unknown density unit (" << (int)dinfo->density_unit << ") **" << std::endl;
      }
    }

    CMS::Profile::ptr profile = jpeg_read_profile(dinfo, dest);
    if (!profile)
      profile = Image::default_profile(format, "file");
    img->set_profile(profile);

    JSAMPROW jpeg_row[1];
    while (dinfo->output_scanline < dinfo->output_height) {
      img->check_rowdata_alloc(dinfo->output_scanline);
      jpeg_row[0] = img->row<unsigned char>(dinfo->output_scanline);
      jpeg_read_scanlines(dinfo, jpeg_row, 1);
      std::cerr << "\r\tRead " << dinfo->output_scanline << " of " << img->height() << " rows";
    }
    std::cerr << "\r\tRead " << img->height() << " of " << img->height() << " rows." << std::endl;

    jpeg_finish_decompress(dinfo);
    jpeg_istream_src_free(dinfo);
    jpeg_destroy_decompress(dinfo);
    free(dinfo);
    _is_open = false;

    std::cerr << "\tExtracting tags..." << std::endl;
    extract_tags(img);

    std::cerr << "Done." << std::endl;
    return img;
  }

  CMS::Format JPEGfile::preferred_format(CMS::Format format) {
    if ((format.colour_model() != CMS::ColourModel::Greyscale)
	&& (format.colour_model() != CMS::ColourModel::CMYK)) {
      format.set_colour_model(CMS::ColourModel::RGB);
    }

    format.set_planar(false);

    format.set_extra_channels(0);

    format.set_8bit();

    return format;
  }

  //! Setup a "destination manager" on the given JPEG compression structure to write to an ostream
  void jpeg_ostream_dest(j_compress_ptr cinfo, std::ostream* os);

  //! Free the data structures of the ostream destination manager
  void jpeg_ostream_dest_free(j_compress_ptr cinfo);

  //! Create a scan "script" for an RGB image
  void jpegfile_scan_RGB(jpeg_compress_struct* cinfo);

  //! Create a scan "script" for a greyscale image
  void jpegfile_scan_greyscale(jpeg_compress_struct* cinfo);

  //! Write an ICC profile into APP2 markers in a JPEG file
  void jpeg_write_profile(jpeg_compress_struct* cinfo, unsigned char *data, unsigned int size);

  void JPEGfile::write(std::ostream& os, Image::ptr img, Destination::ptr dest, bool can_free) {
    jpeg_compress_struct *cinfo = (jpeg_compress_struct*)malloc(sizeof(jpeg_compress_struct));
    jpeg_create_compress(cinfo);
    jpeg_error_mgr jerr;
    cinfo->err = jpeg_std_error(&jerr);

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

    {
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

  void JPEGfile::write(Image::ptr img, Destination::ptr dest, bool can_free) {
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
