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
#include <boost/algorithm/string.hpp>
#include <openjpeg.h>
#include <omp.h>
#include "ImageFile.hh"
#include "Exception.hh"
#include "JP2.hh"

namespace PhotoFinish {

  JP2writer::JP2writer(const fs::path filepath) :
    ImageWriter(filepath)
  {}

  CMS::Format JP2writer::preferred_format(CMS::Format format) {
    if (format.colour_model() != CMS::ColourModel::Greyscale) {
      format.set_colour_model(CMS::ColourModel::RGB);
    }

    format.set_planar();

    format.set_extra_channels(0);
    format.set_premult_alpha();

    if (!format.is_8bit() && !format.is_16bit())
      format.set_16bit();

    return format;
  }

  void JP2writer::write(Image::ptr img, Destination::ptr dest, bool can_free) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    CMS::Format format = img->format();
    if (format.bytes_per_channel() > 2)
      throw cmsTypeError("Too deep", format);

    OPJ_COLOR_SPACE colour_space;
    switch (format.colour_model()) {
    case CMS::ColourModel::RGB:
      colour_space = CLRSPC_SRGB;
      break;

    case CMS::ColourModel::Greyscale:
      colour_space = CLRSPC_GRAY;
      break;

    case CMS::ColourModel::YCbCr:
    case CMS::ColourModel::YUV:
      colour_space = CLRSPC_SYCC;
      break;

    default:
      throw cmsTypeError("Unsupported colour model", format);
      break;
    }

    std::cerr << "Preparing for file " << _filepath << "..." << std::endl;
    std::cerr << "\t" << img->width() << "×" << img->height()
	      << " " << (format.bytes_per_channel() << 3) << "-bpp " << (format.is_planar() ? "planar" : "packed" ) << " " << (colour_space == PT_GRAY ? "greyscale" : "RGB") << std::endl;
    opj_event_mgr_t event_mgr;
    memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
    event_mgr.error_handler = error_callback;
    event_mgr.warning_handler = warning_callback;
    event_mgr.info_handler = info_callback;

    opj_cparameters_t parameters;
    opj_set_default_encoder_parameters(&parameters);

    parameters.tile_size_on = OPJ_FALSE;
    parameters.cp_disto_alloc = 1;

    if (dest->jp2().defined()) {
      D_JP2 d = dest->jp2();
      if (d.numresolutions().defined()) {
	std::cerr << "\tUsing " << d.numresolutions() << " resolutions." << std::endl;
	parameters.numresolution = d.numresolutions();
      }
      if (d.prog_order().defined()) {
	std::string po = d.prog_order().get();
	if (boost::iequals(po, "lrcp")) {
	  std::cerr << "\tLayer-resolution-component-precinct order." << std::endl;
	  parameters.prog_order = LRCP;
	} else if (boost::iequals(po, "rlcp")) {
	  std::cerr << "\tResolution-layer-component-precinct order." << std::endl;
	  parameters.prog_order = RLCP;
	} else if (boost::iequals(po, "rpcl")) {
	  std::cerr << "\tResolution-precinct-component-layer order." << std::endl;
	  parameters.prog_order = RPCL;
	} else if (boost::iequals(po, "pcrl")) {
	  std::cerr << "\tPrecinct-component-resolution-layer order." << std::endl;
	  parameters.prog_order = PCRL;
	} else if (boost::iequals(po, "cprl")) {
	  std::cerr << "\tComponent-precinct-resolution-layer order." << std::endl;
	  parameters.prog_order = CPRL;
	}
      }
      if (d.num_rates() > 0) {
	std::cerr << "\tRate:";
	for (int i = 0; i < d.num_rates(); i++) {
	  parameters.tcp_rates[i] = d.rate(i);
	  if (i > 0)
	    std::cerr << ",";
	  std::cerr << " " << d.rate(i);
	}
	std::cerr << "." << std::endl;
	parameters.tcp_numlayers = d.num_rates();
      }
      /*
      if (d.tile_size().defined()) {
	std::cerr << "\tTile size of " << d.tile_size()->first << "×" << d.tile_size()->second << std::endl;
	parameters.tile_size_on = OPJ_TRUE;
	parameters.cp_tdx = d.tile_size()->first;
	parameters.cp_tdy = d.tile_size()->second;
      }
      */
    }

    if (parameters.tcp_numlayers == 0) {
      parameters.tcp_rates[0] = 0;
      parameters.tcp_numlayers++;
    }

    unsigned char channels = format.channels();
    int depth = format.bytes_per_channel();
    opj_image_cmptparm_t *components = (opj_image_cmptparm_t*)malloc(channels * sizeof(opj_image_cmptparm_t));
    memset(components, 0, channels * sizeof(opj_image_cmptparm_t));
    for (unsigned char i = 0; i < channels; i++) {
      components[i].dx = parameters.subsampling_dx;
      components[i].dy = parameters.subsampling_dy;
      components[i].w = img->width();
      components[i].h = img->height();
      components[i].x0 = components[i].y0 = 0;
      components[i].prec = components[i].bpp = depth << 3;
      components[i].sgnd = 0;
    }
    opj_image_t *jp2_image = opj_image_create(channels, components, colour_space);

    if (jp2_image == NULL)
      return;

    if (img->has_profile()) {
      void *profile_data;
      unsigned int profile_len;
      img->profile()->save_to_mem(profile_data, profile_len);
      if (profile_len > 0) {
	std::cerr << "\tEmbedding profile (" << profile_len << " bytes)." << std::endl;
	jp2_image->icc_profile_buf = (unsigned char*)profile_data;
	jp2_image->icc_profile_len = profile_len;
      }
    }

#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int y = 0; y < img->height(); y++) {
      if (format.is_planar())
	if (depth == 1)
	  write_planar<unsigned char>(img->width(), channels, img->row<unsigned char>(y), jp2_image, y);
	else
	  write_planar<short unsigned int>(img->width(), channels, img->row<short unsigned int>(y), jp2_image, y);
      else
	if (depth == 1)
	  write_packed<unsigned char>(img->width(), channels, img->row<unsigned char>(y), jp2_image, y);
	else
	  write_packed<short unsigned int>(img->width(), channels, img->row<short unsigned int>(y), jp2_image, y);

      if (can_free)
	img->free_row(y);

      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tCopied " << y + 1 << " of " << img->height() << " rows";
    }
    std::cerr << "\r\tCopied " << img->height() << " of " << img->height() << " rows." << std::endl;

    jp2_image->x0 = parameters.image_offset_x0;
    jp2_image->y0 = parameters.image_offset_y0;
    jp2_image->x1 = jp2_image->x0 + (img->width() - 1) * parameters.subsampling_dx + 1;
    jp2_image->y1 = jp2_image->y0 + (img->height() - 1) * parameters.subsampling_dy + 1;

    parameters.tcp_mct = jp2_image->numcomps == 3 ? 1 : 0;

    opj_cinfo_t* cinfo = opj_create_compress(CODEC_JP2);
    opj_set_event_mgr((opj_common_ptr)cinfo, &event_mgr, (void*)this);
    opj_setup_encoder(cinfo, &parameters, jp2_image);
    opj_cio_t *cio = opj_cio_open((opj_common_ptr)cinfo, NULL, 0);
    opj_encode(cinfo, cio, jp2_image, NULL);

    fs::ofstream ofs(_filepath, std::ios_base::out);
    if (ofs.fail())
      throw FileOpenError(_filepath.native());

    {
      int size = cio_tell(cio);
      std::cerr << "\tWriting..." << std::endl;
      char *data = (char*)cio->buffer;
      unsigned int written = 0;
      while (size) {
	unsigned int to_write = size > 1024 ? 1024 : size;
	ofs.write(data, to_write);
	data += to_write;
	written += to_write;
	size -= to_write;
	std::cerr << "\r\tWritten " << written << " bytes";
      }
      std::cerr << "\r\tWritten " << written << " bytes" << std::endl;
      ofs.close();
    }
    opj_cio_close(cio);
    opj_destroy_compress(cinfo);
    opj_image_destroy(jp2_image);
    _is_open = false;

    std::cerr << "\tEmbedding tags..." << std::endl;
    embed_tags(img);

    std::cerr << "Done." << std::endl;
  }

}
