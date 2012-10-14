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
#include "ImageFile.hh"
#include "Exception.hh"

namespace PhotoFinish {

  JP2writer::JP2writer(std::ostream* os, Destination::ptr dest) :
    ImageWriter(os, dest),
    _jp2_image(NULL)
  {
    opj_set_default_encoder_parameters(&_parameters);

    if (_dest->jp2().defined()) {
      D_JP2 d = _dest->jp2();
      if (d.numresolutions().defined()) {
	std::cerr << "\tUsing " << d.numresolutions() << " resolutions." << std::endl;
	_parameters.numresolution = d.numresolutions();
      }
      if (d.prog_order().defined()) {
	std::string po = d.prog_order().get();
	if (boost::iequals(po, "lrcp")) {
	  std::cerr << "\tLayer-resolution-component-precinct order." << std::endl;
	  _parameters.prog_order = LRCP;
	} else if (boost::iequals(po, "rlcp")) {
	  std::cerr << "\tResolution-layer-component-precinct order." << std::endl;
	  _parameters.prog_order = RLCP;
	} else if (boost::iequals(po, "rpcl")) {
	  std::cerr << "\tResolution-precinct-component-layer order." << std::endl;
	  _parameters.prog_order = RPCL;
	} else if (boost::iequals(po, "pcrl")) {
	  std::cerr << "\tPrecinct-component-resolution-layer order." << std::endl;
	  _parameters.prog_order = PCRL;
	} else if (boost::iequals(po, "cprl")) {
	  std::cerr << "\tComponent-precinct-resolution-layer order." << std::endl;
	  _parameters.prog_order = CPRL;
	}
      }
      if (d.num_rates() > 0) {
	std::cerr << "\tRate:";
	for (int i = 0; i < d.num_rates(); i++) {
	  _parameters.tcp_rates[i] = d.rate(i);
	  if (i > 0)
	    std::cerr << ",";
	  std::cerr << " " << d.rate(i);
	}
	std::cerr << "." << std::endl;
	_parameters.tcp_numlayers = d.num_rates();
      }
      if (d.tile_size().defined()) {
	std::cerr << "\tTile size of " << d.tile_size()->first << "×" << d.tile_size()->second << std::endl;
	_parameters.cp_tdx = d.tile_size()->first;
	_parameters.cp_tdy = d.tile_size()->second;
      }
    }

    if (_parameters.tcp_numlayers == 0) {
      _parameters.tcp_rates[0] = 0;
      _parameters.tcp_numlayers++;
    }
    _parameters.cp_disto_alloc = 1;
    _parameters.tile_size_on = 0;
  }

  void JP2writer::receive_image_header(ImageHeader::ptr header) {
    OPJ_COLOR_SPACE colour_space = CLRSPC_SRGB;
    switch (T_COLORSPACE(header->cmsType())) {
    case PT_GRAY:
      colour_space = CLRSPC_GRAY;
      break;

    case PT_RGB:
      colour_space = CLRSPC_SRGB;
      break;

    default:
      break;
    }

    int depth = T_BYTES(header->cmsType()) * 8;

    opj_image_cmptparm_t *components = (opj_image_cmptparm_t*)malloc(T_CHANNELS(header->cmsType()) * sizeof(opj_image_cmptparm_t));
    memset(components, 0, T_CHANNELS(header->cmsType()) * sizeof(opj_image_cmptparm_t));
    for (unsigned char i = 0; i < T_CHANNELS(header->cmsType()); i++) {
      components[i].dx = _parameters.subsampling_dx;
      components[i].dy = _parameters.subsampling_dy;
      components[i].w = header->width();
      components[i].h = header->height();
      components[i].x0 = components[i].y0 = 0;
      components[i].prec = components[i].bpp = depth;
      components[i].sgnd = 0;
    }
    _jp2_image = opj_image_create(T_CHANNELS(header->cmsType()), components, colour_space);

    if (_jp2_image == NULL)
      return;

    _width = header->width();
    _height = header->height();

    cmsUInt32Number intent = INTENT_PERCEPTUAL;	// Default value
    if (_dest->intent().defined())
      intent = _dest->intent();
    this->get_and_embed_profile(header->profile(), header->cmsType(), intent);
  }

  void JP2writer::mark_sGrey(cmsUInt32Number intent) const {}

  void JP2writer::mark_sRGB(cmsUInt32Number intent) const {}

  void JP2writer::embed_icc(std::string name, unsigned char *data, unsigned int len) const {
    _jp2_image->icc_profile_buf = data;
    _jp2_image->icc_profile_len = len;
  }

  //! Read from a row of packed pixel data into OpenJPEG's planar components
  template <typename T>
  inline void packed_to_planar(unsigned int width, unsigned char channels, T* row, opj_image_t* image, unsigned int y) {
    T *in = row;
    unsigned int index = y * width;
    for (unsigned int x = 0; x < width; x++, index++)
      for (unsigned char c = 0; c < channels; c++)
	image->comps[c].data[index] = *in++;
  }

  void error_callback(const char* msg, void* client_data);
  void warning_callback(const char* msg, void* client_data);
  void info_callback(const char* msg, void* client_data);

  void JP2writer::do_work(void) {
    this->_lock_sink_queue();
    ImageRow::ptr row;
    for (_sink_rowqueue_type::iterator rqi = _sink_rowqueue.begin(); rqi != _sink_rowqueue.end(); rqi++)
      if ((*rqi)->y() == _next_y) {
	row = *rqi;
	_sink_rowqueue.erase(rqi);
	break;
      }
    this->_unlock_sink_queue();

    if (row) {
      if (_jp2_image->comps[0].bpp == 8)
	packed_to_planar<unsigned char>(_width, _jp2_image->numcomps, (unsigned char*)row->data(), _jp2_image, _next_y);
      else
	packed_to_planar<unsigned short>(_width, _jp2_image->numcomps, (unsigned short*)row->data(), _jp2_image, _next_y);
      _next_y++;
      if (_next_y == _sink_header->height()) {
	_jp2_image->x0 = _parameters.image_offset_x0;
	_jp2_image->y0 = _parameters.image_offset_y0;
	_jp2_image->x1 = _jp2_image->x0 + (_width - 1) * _parameters.subsampling_dx + 1;
	_jp2_image->y1 = _jp2_image->y0 + (_height - 1) * _parameters.subsampling_dy + 1;

	_parameters.tcp_mct = _jp2_image->numcomps > 1 ? 1 : 0;

	opj_event_mgr_t event_mgr;
	memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
	event_mgr.error_handler = error_callback;
	event_mgr.warning_handler = warning_callback;
	event_mgr.info_handler = info_callback;

	opj_cinfo_t* cinfo = opj_create_compress(CODEC_JP2);
	opj_set_event_mgr((opj_common_ptr)cinfo, &event_mgr, (void*)this);
	opj_setup_encoder(cinfo, &_parameters, _jp2_image);
	opj_cio_t *cio = opj_cio_open((opj_common_ptr)cinfo, NULL, 0);
	opj_encode(cinfo, cio, _jp2_image, NULL);

	int size = cio_tell(cio);
	std::cerr << "\tWriting " << size << " bytes..." << std::endl;
	_os->write((char*)cio->buffer, size);
	opj_cio_close(cio);
	opj_destroy_compress(cinfo);
	opj_image_destroy(_jp2_image);
	_jp2_image = NULL;
	this->_set_work_finished();
      }
    }
  }

  void JP2writer::receive_image_end(void) {
  }

}
