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
#include <lcms2.h>
#include <omp.h>
#include "ImageFile.hh"
#include "Exception.hh"

namespace PhotoFinish {

  JP2file::JP2file(const fs::path filepath) :
    ImageFile(filepath)
  {}

  //! Error callback for OpenJPEG - throw a LibraryError exception
  void error_callback(const char* msg, void* client_data) {
    throw LibraryError("OpenJPEG", msg);
  }

  //! Warning callback for OpenJPEG - print the message to STDERR
  void warning_callback(const char* msg, void* client_data) {
    ((char*)msg)[strlen(msg) - 1] = 0;
    std::cerr << "** OpenJPEG:" << msg << " **" << std::endl;
  }

  //! Info callback for OpenJPEG - print the indented message to STDERR
  void info_callback(const char* msg, void* client_data) {
    ((char*)msg)[strlen(msg) - 1] = 0;
    std::cerr << "\tOpenJPEG:" << msg << std::endl;
  }

  //! Read a row of image data from OpenJPEG's planar integer components into an LCMS2-compatible single array
  template <typename T>
  inline void read_planar(unsigned int width, unsigned char channels, opj_image_t* image, T* row, unsigned int y) {
    T *out = row;
    unsigned int index_start = y * width;
    for (unsigned char c = 0; c < channels; c++) {
      unsigned int index = index_start;
      for (unsigned int x = 0; x < width; x++, out++, index++)
	*out = image->comps[c].data[index];
    }
  }

  //! Read a row of planar pixel data into OpenJPEG's planar components
  template <typename T>
  void write_planar(unsigned int width, unsigned char channels, T* row, opj_image_t* image, unsigned int y) {
    T *in = row;
    unsigned int index_start = y * width;
    for (unsigned char c = 0; c < channels; c++) {
      unsigned int index = index_start;
      for (unsigned int x = 0; x < width; x++, index++, in++)
	image->comps[c].data[index] = *in;
    }
  }

  Image::ptr JP2file::read(Destination::ptr dest) {
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
    cmsUInt32Number type = PLANAR_SH(1) | CHANNELS_SH(jp2_image->numcomps) | BYTES_SH(depth >> 3);
    switch (jp2_image->color_space) {
    case CLRSPC_SRGB:
      type |= COLORSPACE_SH(PT_RGB);
      break;

    case CLRSPC_GRAY:
      type |= COLORSPACE_SH(PT_GRAY);
      break;

    case CLRSPC_SYCC:
      type |= COLORSPACE_SH(PT_YUV);
      break;

    default:
      std::cerr << "** Unknown colour space " << jp2_image->color_space << " **" << std::endl;
      throw LibraryError("OpenJPEG", "color_space");
    }
    dest->set_depth(depth);

    auto img = std::make_shared<Image>(jp2_image->x1 - jp2_image->x0, jp2_image->y1 - jp2_image->y0, type);

    cmsHPROFILE profile;
    if (jp2_image->icc_profile_buf != NULL) {
      profile = cmsOpenProfileFromMem(jp2_image->icc_profile_buf, jp2_image->icc_profile_len);
      void *data_copy = malloc(jp2_image->icc_profile_len);
      memcpy(data_copy, jp2_image->icc_profile_buf, jp2_image->icc_profile_len);
      char *profile_name = NULL;
      unsigned int profile_name_len;
      if ((profile_name_len = cmsGetProfileInfoASCII(profile, cmsInfoDescription, "en", cmsNoCountry, NULL, 0)) > 0) {
	profile_name = (char*)malloc(profile_name_len);
	cmsGetProfileInfoASCII(profile, cmsInfoDescription, "en", cmsNoCountry, profile_name, profile_name_len);
	dest->set_profile(profile_name, data_copy, jp2_image->icc_profile_len);
	free(profile_name);
      } else
	dest->set_profile("JP2 ICC profile", data_copy, jp2_image->icc_profile_len);
      img->set_profile(profile);
    }

#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int y = 0; y < img->height(); y++) {
      for (unsigned char c = 0; c < T_CHANNELS(type); c++)
	if (depth == 1)
	  read_planar<unsigned char>(img->width(), T_CHANNELS(type), jp2_image, img->row(y), y);
	else
	  read_planar<short unsigned int>(img->width(), T_CHANNELS(type), jp2_image, (short unsigned int*)img->row(y), y);

      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tCopied " << (y + 1) << " of " << img->height() << " rows";
    }
    std::cerr << "\r\tCopied " << img->height() << " of " << img->height() << " rows." << std::endl;
    _is_open = false;

    std::cerr << "Done." << std::endl;
    return img;
  }

  cmsUInt32Number JP2file::preferred_type(cmsUInt32Number type) {
    type &= FLOAT_MASK;

    if (T_COLORSPACE(type) != PT_GRAY) {
      type &= COLORSPACE_MASK;
      type |= COLORSPACE_SH(PT_RGB);
      type &= CHANNELS_MASK;
      type |= CHANNELS_SH(3);
    }

    type &= PLANAR_MASK;
    type |= PLANAR_SH(1);

    type &= EXTRA_MASK;

    if ((T_BYTES(type) == 0) || (T_BYTES(type) > 2)) {
      type &= BYTES_MASK;
      type |= BYTES_SH(2);
    }

    return type;
  }

  void JP2file::write(Image::ptr img, Destination::ptr dest, bool can_free) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    cmsUInt32Number type = img->type();
    if (T_PLANAR(type) != 1)
      throw cmsTypeError("Not Planar", type);

    OPJ_COLOR_SPACE colour_space;
    switch (T_COLORSPACE(type)) {
    case PT_RGB:
      colour_space = CLRSPC_SRGB;
      break;

    case PT_GRAY:
      colour_space = CLRSPC_GRAY;
      break;

    case PT_YUV:
      colour_space = CLRSPC_SYCC;
      break;

    default:
      throw cmsTypeError("Unsupported colour space", type);
      break;
    }
    unsigned char channels = T_CHANNELS(type);
    int depth = T_BYTES(type);

    std::cerr << "Preparing for file " << _filepath << "..." << std::endl;
    opj_event_mgr_t event_mgr;
    memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
    event_mgr.error_handler = error_callback;
    event_mgr.warning_handler = warning_callback;
    event_mgr.info_handler = info_callback;

    opj_cparameters_t parameters;
    opj_set_default_encoder_parameters(&parameters);

    parameters.tile_size_on = 0;
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
      if (d.tile_size().defined()) {
	std::cerr << "\tTile size of " << d.tile_size()->first << "×" << d.tile_size()->second << std::endl;
	parameters.tile_size_on = 1;
	parameters.cp_tdx = d.tile_size()->first;
	parameters.cp_tdy = d.tile_size()->second;
      }
    }

    if (parameters.tcp_numlayers == 0) {
      parameters.tcp_rates[0] = 0;
      parameters.tcp_numlayers++;
    }

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

    {
      unsigned char *profile_data = NULL;
      unsigned int profile_len = 0;
      cmsSaveProfileToMem(img->profile(), NULL, &profile_len);
      if (profile_len > 0) {
	profile_data = (unsigned char*)malloc(profile_len);
	cmsSaveProfileToMem(img->profile(), profile_data, &profile_len);

	std::cerr << "\tEmbedding profile (" << profile_len << " bytes)." << std::endl;
	jp2_image->icc_profile_buf = profile_data;
	jp2_image->icc_profile_len = profile_len;
      }
    }

    unsigned char **rows = (unsigned char**)malloc(img->height() * sizeof(unsigned char*));
    size_t channel_size = img->width() * depth;
    size_t row_size = channel_size * channels;
    for (unsigned int y = 0; y < img->height(); y++)
      rows[y] = (unsigned char*)malloc(row_size);

#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int y = 0; y < img->height(); y++) {
      if (depth == 1)
	write_planar<unsigned char>(img->width(), channels, img->row(y), jp2_image, y);
      else
	write_planar<short unsigned int>(img->width(), channels, (short unsigned int*)img->row(y), jp2_image, y);

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

    int size = cio_tell(cio);
    std::cerr << "\tWriting " << size << " bytes..." << std::endl;
    ofs.write((char*)cio->buffer, size);
    ofs.close();
    opj_cio_close(cio);
    opj_destroy_compress(cinfo);
    opj_image_destroy(jp2_image);
    _is_open = false;

    std::cerr << "Done." << std::endl;
  }

}
