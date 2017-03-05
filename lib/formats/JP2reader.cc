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
#include <boost/algorithm/string.hpp>
#include <openjpeg.h>
#include <omp.h>
#include "ImageFile.hh"
#include "Exception.hh"
#include "JP2.hh"

namespace PhotoFinish {

  JP2reader::JP2reader(const fs::path filepath) :
    ImageReader(filepath)
  {}

  Image::ptr JP2reader::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    opj_event_mgr_t event_mgr;
    memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
    event_mgr.error_handler = error_callback;
    event_mgr.warning_handler = warning_callback;
    event_mgr.info_handler = info_callback;

    opj_dparameters_t parameters;
    opj_set_default_decoder_parameters(&parameters);

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream ifs(_filepath, std::ios_base::in);
    if (ifs.fail())
      throw FileOpenError(_filepath.native());

    unsigned int file_size = fs::file_size(_filepath);
    unsigned char *src = (unsigned char *)malloc(file_size);
    ifs.read((char*)src, file_size);

    opj_dinfo_t *dinfo = opj_create_decompress(CODEC_JP2);
    opj_set_event_mgr((opj_common_ptr)dinfo, &event_mgr, (void*)this);
    opj_setup_decoder(dinfo, &parameters);
    opj_cio_t *cio = opj_cio_open((opj_common_ptr)dinfo, src, file_size);
    opj_image_t *jp2_image = opj_decode(dinfo, cio);
    if (jp2_image == NULL)
      throw LibraryError("OpenJPEG", "Could not decode file");

    opj_cio_close(cio);
    free(src);

    // Is this necessary?
    if (jp2_image->numcomps > 1)
      for (unsigned char c = 1; c < jp2_image->numcomps; c++) {
	if (jp2_image->comps[c].dx != jp2_image->comps[0].dx)
	  std::cerr << "** Component " << (int)c << " has a different dx to the first! **" << std::endl;
	if (jp2_image->comps[c].dy != jp2_image->comps[0].dy)
	  std::cerr << "** Component " << (int)c << " has a different dy to the first! **" << std::endl;
	if (jp2_image->comps[c].w != jp2_image->comps[0].w)
	  std::cerr << "** Component " << (int)c << " has a different w to the first! **" << std::endl;
	if (jp2_image->comps[c].h != jp2_image->comps[0].h)
	  std::cerr << "** Component " << (int)c << " has a different h to the first! **" << std::endl;
	if (jp2_image->comps[c].x0 != jp2_image->comps[0].x0)
	  std::cerr << "** Component " << (int)c << " has a different x0 to the first! **" << std::endl;
	if (jp2_image->comps[c].y0 != jp2_image->comps[0].y0)
	  std::cerr << "** Component " << (int)c << " has a different y0 to the first! **" << std::endl;
	if (jp2_image->comps[c].prec != jp2_image->comps[0].prec)
	  std::cerr << "** Component " << (int)c << " has a different prec to the first! **" << std::endl;
	if (jp2_image->comps[c].bpp != jp2_image->comps[0].bpp)
	  std::cerr << "** Component " << (int)c << " has a different bpp to the first! **" << std::endl;
	if (jp2_image->comps[c].sgnd != jp2_image->comps[0].sgnd)
	  std::cerr << "** Component " << (int)c << " has a different sgnd to the first! **" << std::endl;
      }

    int depth = jp2_image->comps[0].prec;
    CMS::Format format;
    format.set_planar();
    switch (depth >> 3) {
    case 1: format.set_8bit();
      break;

    case 2: format.set_16bit();
      break;

    case 4: format.set_32bit();
      break;

    default:
      std::cerr << "** Unknown depth " << (depth >> 3) << " **" << std::endl;
      throw LibraryError("OpenJPEG", "depth");
    }

    switch (jp2_image->color_space) {
    case CLRSPC_SRGB:
      format.set_colour_model(CMS::ColourModel::RGB, jp2_image->numcomps);
      break;

    case CLRSPC_GRAY:
      format.set_colour_model(CMS::ColourModel::Greyscale, jp2_image->numcomps);
      break;

    case CLRSPC_SYCC:
      format.set_colour_model(CMS::ColourModel::YUV, jp2_image->numcomps);
      break;

    default:
      std::cerr << "** Unknown colour space " << jp2_image->color_space << " **" << std::endl;
      throw LibraryError("OpenJPEG", "color_space");
    }
    dest->set_depth(depth);

    auto img = std::make_shared<Image>(jp2_image->x1 - jp2_image->x0, jp2_image->y1 - jp2_image->y0, format);

    if (jp2_image->icc_profile_buf != NULL) {
      CMS::Profile::ptr profile = std::make_shared<CMS::Profile>(jp2_image->icc_profile_buf, jp2_image->icc_profile_len);
      unsigned char *data_copy = new unsigned char[jp2_image->icc_profile_len];
      memcpy(data_copy, jp2_image->icc_profile_buf, jp2_image->icc_profile_len);

      std::string profile_name = profile->description("en", "");
      if (profile_name.length() > 0)
	dest->set_profile(profile_name, data_copy, jp2_image->icc_profile_len);
      else
	dest->set_profile("JP2 ICC profile", data_copy, jp2_image->icc_profile_len);
      img->set_profile(profile);
    }

#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int y = 0; y < img->height(); y++) {
      img->check_rowdata_alloc(y);
      for (unsigned char c = 0; c < format.channels(); c++)
	if (depth == 1)
	  read_planar<unsigned char>(img->width(), format.channels(), jp2_image, img->row<unsigned char>(y), y);
	else
	  read_planar<short unsigned int>(img->width(), format.channels(), jp2_image, img->row<short unsigned int>(y), y);

      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tCopied " << (y + 1) << " of " << img->height() << " rows";
    }
    std::cerr << "\r\tCopied " << img->height() << " of " << img->height() << " rows." << std::endl;
    _is_open = false;

    std::cerr << "\tExtracting tags..." << std::endl;
    extract_tags(img);

    std::cerr << "Done." << std::endl;
    return img;
  }

}
