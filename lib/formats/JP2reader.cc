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

  void error_callback(const char* msg, void* client_data);
  void warning_callback(const char* msg, void* client_data);
  void info_callback(const char* msg, void* client_data);

  JP2reader::JP2reader(std::istream* is) :
    ImageReader(is),
    _dinfo(NULL),
    _src(NULL), _cio(NULL),
    _jp2_image(NULL),
    _width(0), _height(0), _next_y(0)
  {
    opj_event_mgr_t event_mgr;
    memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
    event_mgr.error_handler = error_callback;
    event_mgr.warning_handler = warning_callback;
    event_mgr.info_handler = info_callback;

    opj_dparameters_t parameters;
    opj_set_default_decoder_parameters(&parameters);

    _dinfo = opj_create_decompress(CODEC_JP2);
    opj_set_event_mgr((opj_common_ptr)_dinfo, &event_mgr, (void*)this);
    opj_setup_decoder(_dinfo, &parameters);
  }

  //! Read a row of image data from OpenJPEG's planar components into a packed pixel array
  template <typename T>
  inline void* planar_to_packed(unsigned int width, unsigned char channels, opj_image_t* image, unsigned int y) {
    T *row = (T*)malloc(width * channels * sizeof(T));
    T *out = row;
    unsigned int index = y * width;
    for (unsigned int x = 0; x < width; x++, index++)
      for (unsigned char c = 0; c < channels; c++)
	*out++ = image->comps[c].data[index];
    return (void*)row;
  }

  void JP2reader::do_work(void) {
    if (!this->_test_reader_lock())
      return;

    switch (_read_state) {
    case 0:
      {
	_is->seekg(0, std::ios_base::end);
	unsigned int file_size = _is->tellg();
	_is->seekg(0, std::ios_base::beg);
	_src = (unsigned char *)malloc(file_size);
	_is->read((char*)_src, file_size);

	_cio = opj_cio_open((opj_common_ptr)_dinfo, _src, file_size);
	_jp2_image = opj_decode(_dinfo, _cio);
	if (_jp2_image == NULL)
	  throw LibraryError("OpenJPEG", "Could not decode file");

	// Is this necessary?
	if (_jp2_image->numcomps > 1)
	  for (unsigned char c = 1; c < _jp2_image->numcomps; c++) {
	    if (_jp2_image->comps[c].dx != _jp2_image->comps[0].dx)
	      std::cerr << "** Component " << (int)c << " has a different dx to the first! **" << std::endl;
	    if (_jp2_image->comps[c].dy != _jp2_image->comps[0].dy)
	      std::cerr << "** Component " << (int)c << " has a different dy to the first! **" << std::endl;
	    if (_jp2_image->comps[c].w != _jp2_image->comps[0].w)
	      std::cerr << "** Component " << (int)c << " has a different w to the first! **" << std::endl;
	    if (_jp2_image->comps[c].h != _jp2_image->comps[0].h)
	      std::cerr << "** Component " << (int)c << " has a different h to the first! **" << std::endl;
	    if (_jp2_image->comps[c].x0 != _jp2_image->comps[0].x0)
	      std::cerr << "** Component " << (int)c << " has a different x0 to the first! **" << std::endl;
	    if (_jp2_image->comps[c].y0 != _jp2_image->comps[0].y0)
	      std::cerr << "** Component " << (int)c << " has a different y0 to the first! **" << std::endl;
	    if (_jp2_image->comps[c].prec != _jp2_image->comps[0].prec)
	      std::cerr << "** Component " << (int)c << " has a different prec to the first! **" << std::endl;
	    if (_jp2_image->comps[c].bpp != _jp2_image->comps[0].bpp)
	      std::cerr << "** Component " << (int)c << " has a different bpp to the first! **" << std::endl;
	    if (_jp2_image->comps[c].sgnd != _jp2_image->comps[0].sgnd)
	      std::cerr << "** Component " << (int)c << " has a different sgnd to the first! **" << std::endl;
	  }

	_width = _jp2_image->x1 - _jp2_image->x0;
	_height = _jp2_image->y1 - _jp2_image->y0;
	ImageHeader::ptr header(new ImageHeader(_width, _height));

	int depth = _jp2_image->comps[0].prec;
	cmsUInt32Number cmsType = CHANNELS_SH(_jp2_image->numcomps) | BYTES_SH(depth >> 3);
	switch (_jp2_image->color_space) {
	case CLRSPC_SRGB:
	  cmsType |= COLORSPACE_SH(PT_RGB);
	  break;

	case CLRSPC_GRAY:
	  cmsType |= COLORSPACE_SH(PT_GRAY);
	  break;

	case CLRSPC_SYCC:
	  cmsType |= COLORSPACE_SH(PT_YUV);
	  break;

	default:
	  std::cerr << "** Unknown colour space " << _jp2_image->color_space << " **" << std::endl;
	  throw LibraryError("OpenJPEG", "color_space");
	}

	cmsHPROFILE profile;
	if (_jp2_image->icc_profile_buf != NULL) {
	  profile = cmsOpenProfileFromMem(_jp2_image->icc_profile_buf, _jp2_image->icc_profile_len);
	  header->set_profile(profile);
	} else
	  header->set_profile(default_profile(cmsType));

	this->_send_image_header(header);
	_read_state = 1;
      }
      break;

    case 1:
      if (_next_y < _height) {
	void *buffer = NULL;
	if (_jp2_image->comps[0].prec == 8)
	  buffer = planar_to_packed<unsigned char>(_width, _jp2_image->numcomps, _jp2_image, _next_y);
	else
	  buffer = planar_to_packed<unsigned short>(_width, _jp2_image->numcomps, _jp2_image, _next_y);
	ImageRow::ptr row(new ImageRow(_next_y, buffer));
	_next_y++;
	this->_send_image_row(row);
      } else
	_read_state = 2;
      break;

    case 2:
      opj_cio_close(_cio);
      free(_src);

      this->_send_image_end();
      _read_state = 99;
      break;

    default:
      break;
    }
    this->_unlock_reader();
  }

}
