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
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "Image.hh"
#include "ImageFile.hh"

namespace PhotoFinish {

  Image::Image(unsigned int w, unsigned int h, CMS::Format f) :
    _width(w),
    _height(h),
    _profile(NULL),
    _format(f),
    _row_size(0),
    _rowdata(NULL)
  {
    _pixel_size = _format.bytes_per_pixel();
    _row_size = _width * _pixel_size;

    _rowdata = (unsigned char**)malloc(_height * sizeof(unsigned char*));
    for (unsigned int y = 0; y < _height; y++)
      _rowdata[y] = NULL;
  }

  Image::~Image() {
    if (_rowdata != NULL) {
      for (unsigned int y = 0; y < _height; y++)
	if (_rowdata[y] != NULL) {
	  free(_rowdata[y]);
	  _rowdata[y] = NULL;
	}
      free(_rowdata);
      _rowdata = NULL;
    }
  }

  CMS::Profile::ptr Image::default_profile(CMS::ColourModel default_colourmodel, std::string for_desc) {
    CMS::Profile::ptr profile = NULL;
    switch (default_colourmodel) {
    case CMS::ColourModel::RGB:
      std::cerr << "\tUsing default sRGB profile for " << for_desc << "." << std::endl;
      return CMS::Profile::sRGB();
      break;

    case CMS::ColourModel::Greyscale:
      std::cerr << "\tUsing default greyscale profile for " << for_desc << "." << std::endl;
      return CMS::Profile::sGrey();
      break;

    case CMS::ColourModel::Lab:
      std::cerr << "\tUsing default Lab profile for " << for_desc << "." << std::endl;
      return CMS::Profile::Lab4();
      break;

    default:
      std::cerr << "** Cannot assign a default profile for colour model " << default_colourmodel << " **" << std::endl;
    }

    return profile;
  }

  template <typename A, typename B>
  void transfer_alpha_typed2(unsigned int width, unsigned char src_channels, const A* src_row, unsigned char dest_channels, const B* dest_row) {
    double factor = (double)scaleval<B>() / scaleval<A>();
    A *inp = const_cast<A*>(src_row) + src_channels;
    B *outp = const_cast<B*>(dest_row) + dest_channels;
    for (unsigned int x = width; x; x--, inp += 1 + src_channels, outp += 1 + dest_channels)
      *outp = limitval<B>((*inp) * factor);
  }

  template <typename A>
  void transfer_alpha_typed(unsigned int width, unsigned char src_channels, const A* src_row, CMS::Format dest_format, const void* dest_row) {
    unsigned char dest_channels = (unsigned char)dest_format.channels();
    switch (dest_format.bytes_per_channel()) {
    case 1:
      transfer_alpha_typed2<A, unsigned char>(width, src_channels, src_row, dest_channels, (unsigned char*)dest_row);
      break;

    case 2:
      transfer_alpha_typed2<A, short unsigned int>(width, src_channels, src_row, dest_channels,(short unsigned int*)dest_row);
      break;

    case 4:
      if (dest_format.is_fp())
	transfer_alpha_typed2<A, float>(width, src_channels, src_row, dest_channels,(float*)dest_row);
      else
	transfer_alpha_typed2<A, unsigned int>(width, src_channels, src_row, dest_channels,(unsigned int*)dest_row);
      break;

    case 8:
      transfer_alpha_typed2<A, double>(width, src_channels, src_row, dest_channels,(double*)dest_row);
      break;

    }

  }

  void transfer_alpha(unsigned int width, CMS::Format src_format, const void* src_row, CMS::Format dest_format, const void* dest_row) {
    unsigned char src_channels = (unsigned char)src_format.channels();
    switch (src_format.bytes_per_channel()) {
    case 1:
      transfer_alpha_typed<unsigned char>(width, src_channels, (unsigned char*)src_row, dest_format, dest_row);
      break;

    case 2:
      transfer_alpha_typed<short unsigned int>(width, src_channels, (short unsigned int*)src_row, dest_format, dest_row);
      break;

    case 4:
      if (src_format.is_fp())
	transfer_alpha_typed<float>(width, src_channels, (float*)src_row, dest_format, dest_row);
      else
	transfer_alpha_typed<unsigned int>(width, src_channels, (unsigned int*)src_row, dest_format, dest_row);
      break;

    case 8:
      transfer_alpha_typed<double>(width, src_channels, (double*)src_row, dest_format, dest_row);
      break;
    }
  }


  std::string profile_name(CMS::Profile::ptr profile) {
    return profile->description("en", "");
  }

  Image::ptr Image::transform_colour(CMS::Profile::ptr dest_profile, CMS::Format dest_format, CMS::Intent intent, bool can_free) {
    CMS::Profile::ptr profile = _profile;
    if (_profile == NULL)
      profile = default_profile(_format, "source");
    if (dest_profile == NULL)
      dest_profile = profile;

    CMS::Format orig_dest_format = dest_format;
    bool need_alpha_mult = false;
    if (_format.extra_channels() > 0) {
      if (_format.is_premult_alpha() && (!dest_format.is_premult_alpha()))
	this->un_alpha_mult();
      else if ((!_format.is_premult_alpha()) && dest_format.is_premult_alpha()) {
	dest_format.set_channel_type(_format);
	dest_format.set_extra_channels(_format.extra_channels());
	dest_format.set_packed();
	dest_format.unset_premult_alpha();
	need_alpha_mult = true;
      }
    }

#pragma omp parallel
    {
#pragma omp master
      {
	std::cerr << "Transforming colour from \"" << profile_name(profile) << "\" (" << _format << ") to \"" << profile_name(dest_profile) << "\" (" << dest_format << ") using " << omp_get_num_threads() << " threads..." << std::endl;
      }
    }

    CMS::Transform transform(profile, _format,
			     dest_profile, dest_format,
			     intent, 0);

    auto dest = std::make_shared<Image>(_width, _height, dest_format);
    dest->set_profile(dest_profile);
    dest->set_resolution(_xres, _yres);

#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int y = 0; y < _height; y++) {
      dest->check_rowdata_alloc(y);
      transform.transform_buffer(_rowdata[y], dest->row(y), _width);
      if (dest_format.extra_channels())
	transfer_alpha(_width, _format, row(y), dest_format, dest->row(y));

      if (can_free)
	this->free_row(y);
      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tTransformed " << y + 1 << " of " << _height << " rows";
    }
    std::cerr << "\r\tTransformed " << _height << " of " << _height << " rows." << std::endl;


    if (need_alpha_mult)
      dest->alpha_mult(orig_dest_format);

    return dest;
  }

  void Image::transform_colour_inplace(CMS::Profile::ptr dest_profile, CMS::Format dest_format, CMS::Intent intent) {
    CMS::Profile::ptr profile = _profile;
    if (_profile == NULL)
      profile = default_profile(_format, "source");
    if (dest_profile == NULL)
      dest_profile = profile;

    CMS::Format orig_dest_format = dest_format;
    bool need_alpha_mult = false;
    if (_format.extra_channels() > 0) {
      if (_format.is_premult_alpha() && (!dest_format.is_premult_alpha()))
	this->un_alpha_mult();
      else if ((!_format.is_premult_alpha()) && dest_format.is_premult_alpha()) {
	dest_format.set_channel_type(_format);
	dest_format.set_extra_channels(_format.extra_channels());
	dest_format.set_packed();
	dest_format.unset_premult_alpha();
	need_alpha_mult = true;
      }
    }

#pragma omp parallel
    {
#pragma omp master
      {
	std::cerr << "Transforming colour in-place from \"" << profile_name(profile) << "\" (" << _format << ") to \"" << profile_name(dest_profile) << "\" (" << dest_format << ") using " << omp_get_num_threads() << " threads..." << std::endl;
      }
    }

    CMS::Transform transform(profile, _format,
			     dest_profile, dest_format,
			     intent, 0);
    size_t dest_pixel_size = dest_format.bytes_per_pixel();
    size_t dest_row_size = _width * dest_pixel_size;

#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int y = 0; y < _height; y++) {
      void *src_rowdata = row(y);
      void *dest_rowdata = malloc(dest_row_size);
      transform.transform_buffer(src_rowdata, dest_rowdata, _width);
      if (dest_format.extra_channels())
	transfer_alpha(_width, _format, src_rowdata, dest_format, dest_rowdata);

      _rowdata[y] = (unsigned char*)dest_rowdata;
      free(src_rowdata);

      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tTransformed " << y + 1 << " of " << _height << " rows";
    }
    std::cerr << "\r\tTransformed " << _height << " of " << _height << " rows." << std::endl;

    _profile = dest_profile;
    _format = dest_format;
    _pixel_size = dest_pixel_size;
    _row_size = dest_row_size;

    if (need_alpha_mult)
      alpha_mult(orig_dest_format);
  }

  template <typename SRC>
  void Image::_un_alpha_mult_src(void) {
    CMS::Format dest_format = _format;
    SET_SAMPLE_FORMAT(dest_format);

#pragma omp parallel
    {
#pragma omp master
      {
	std::cerr << "Un-multiplying colour from the alpha channel and transforming into " << dest_format << " (scale=" << scaleval<SAMPLE>() << "/" << (SAMPLE)scaleval<SRC>() << ") using " << omp_get_num_threads() << " threads..." << std::endl;
      }
    }

    size_t dest_pixel_size = dest_format.bytes_per_pixel();
    size_t dest_row_size = _width * dest_pixel_size;
    unsigned char alphachan = _format.channels();
    SAMPLE scale = scaleval<SAMPLE>() / scaleval<SRC>();

#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int y = 0; y < _height; y++) {
      void *src_rowdata = row(y);
      void *dest_rowdata = malloc(dest_row_size);

      SRC *in = (SRC*)src_rowdata;
      SAMPLE *out = (SAMPLE*)dest_rowdata;
      for (unsigned int x = 0; x < _width; x++, in += _format.total_channels(), out += dest_format.total_channels()) {
	SRC alpha = in[alphachan];
	unsigned char c;
	if (alpha > 0) {
	  SAMPLE recip_alpha = scaleval<SAMPLE>() / alpha;
	  for (c = 0; c < alphachan; c++)
	    out[c] = limitval<SAMPLE>(in[c] * recip_alpha);
	} else
	  for (c = 0; c < alphachan; c++)
	    out[c] = 0; // or scaleval<SAMPLE>() ?
	for (; c < _format.total_channels(); c++)
	  out[c] = limitval<SAMPLE>(in[c] * scale);
      }

      _rowdata[y] = (unsigned char*)dest_rowdata;
      free(src_rowdata);

      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tTransformed " << y + 1 << " of " << _height << " rows";
    }
    std::cerr << "\r\tTransformed " << _height << " of " << _height << " rows." << std::endl;

    _format = dest_format;
    _format.unset_premult_alpha();
    _pixel_size = dest_pixel_size;
    _row_size = dest_row_size;
  }

  void Image::un_alpha_mult(void) {
    if ((_format.extra_channels() > 0) && _format.is_premult_alpha()) {
      if (_format.is_8bit())
	_un_alpha_mult_src<unsigned char>();
      else if (_format.is_16bit())
	_un_alpha_mult_src<short unsigned int>();
      else if (_format.is_32bit())
	_un_alpha_mult_src<unsigned int>();
      else if (_format.is_float())
	_un_alpha_mult_src<float>();
      else
	_un_alpha_mult_src<double>();
    } else
      std::cerr << "CPAG::Image::un_alpha_mult: format=" << _format << std::endl;
  }

  template <typename SRC, typename DST>
  void Image::_alpha_mult_src_dst(CMS::Format dest_format) {
#pragma omp parallel
    {
#pragma omp master
      {
	std::cerr << "Multiplying colour from the alpha channel and transforming into " << dest_format << " (scale=" << (SAMPLE)scaleval<DST>() << "/" << (SAMPLE)scaleval<SRC>() << ") using " << omp_get_num_threads() << " threads..." << std::endl;
      }
    }

    size_t dest_pixel_size = dest_format.bytes_per_pixel();
    size_t dest_row_size = _width * dest_pixel_size;
    unsigned char alphachan = _format.channels();
    SAMPLE scale = (SAMPLE)scaleval<DST>() / scaleval<SRC>();
    SAMPLE src_scale = scale / scaleval<SRC>();

#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int y = 0; y < _height; y++) {
      void *src_rowdata = row(y);
      void *dest_rowdata = malloc(dest_row_size);

      SRC *in = (SRC*)src_rowdata;
      DST *out = (DST*)dest_rowdata;
      for (unsigned int x = 0; x < _width; x++, in += _format.total_channels(), out += dest_format.total_channels()) {
	SAMPLE alpha = in[alphachan] * src_scale;
	unsigned char c;
	for (c = 0; c < alphachan; c++) {
	  out[c] = limitval<DST>(in[c] * alpha);
	}
	for (; c < dest_format.total_channels(); c++)
	  out[c] = limitval<DST>(in[c] * scale);
      }

      _rowdata[y] = (unsigned char*)dest_rowdata;
      free(src_rowdata);

      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tTransformed " << y + 1 << " of " << _height << " rows";
    }
    std::cerr << "\r\tTransformed " << _height << " of " << _height << " rows." << std::endl;

    _format.set_channel_type(dest_format);
    _format.set_premult_alpha();
    _format.set_packed();
    _pixel_size = dest_pixel_size;
    _row_size = dest_row_size;
  }

  template <typename SRC>
  void Image::_alpha_mult_src(CMS::Format dest_format) {
    // We only take the channel type (bytes and float flag) and number of extra channels from dest_format
    {
      CMS::Format temp_format = _format;
      temp_format.set_channel_type(dest_format);
      temp_format.set_extra_channels(dest_format.extra_channels());
      dest_format = temp_format;
    }

    if (dest_format.is_8bit())
      _alpha_mult_src_dst<SRC, unsigned char>(dest_format);
    else if (dest_format.is_16bit())
      _alpha_mult_src_dst<SRC, short unsigned int>(dest_format);
    else if (dest_format.is_32bit())
      _alpha_mult_src_dst<SRC, unsigned int>(dest_format);
    else if (dest_format.is_float())
      _alpha_mult_src_dst<SRC, float>(dest_format);
    else
      _alpha_mult_src_dst<SRC, double>(dest_format);

  }

  void Image::alpha_mult(CMS::Format dest_format) {
    if ((_format.extra_channels() > 0) && !_format.is_premult_alpha()) {
      if (_format.is_8bit())
	_alpha_mult_src<unsigned char>(dest_format);
      else if (_format.is_16bit())
	_alpha_mult_src<short unsigned int>(dest_format);
      else if (_format.is_32bit())
	_alpha_mult_src<unsigned int>(dest_format);
      else if (_format.is_float())
	_alpha_mult_src<float>(dest_format);
      else
	_alpha_mult_src<double>(dest_format);
    } else
      std::cerr << "CPAG::Image::alpha_mult: format=" << _format << std::endl;
  }


} // namespace CPAG
