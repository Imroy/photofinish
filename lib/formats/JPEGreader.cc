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
#include "JPEG.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  JPEGreader::JPEGreader(const fs::path filepath) :
    ImageReader(filepath)
  {}

  Image::ptr JPEGreader::read(Destination::ptr dest) {
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

}
