/*
	Copyright 2014-2019 Ian Tester

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
#include "Benchmark.hh"

namespace PhotoFinish {

  Image::Image(unsigned int w, unsigned int h, CMS::Format f) :
    _width(w),
    _height(h),
    _format(f),
    _row_size(0),
    _rows(h, nullptr)
  {
    _pixel_size = _format.bytes_per_pixel();
    _row_size = _width * _pixel_size;
  }

  Image::~Image() {
    _rows.clear();
  }

  CMS::Profile::ptr Image::default_profile(CMS::ColourModel default_colourmodel, std::string for_desc) {
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

    return std::make_shared<CMS::Profile>();
  }

  void Image::replace_row(std::shared_ptr<ImageRow> newrow) {
    if (newrow->_image == this)
      _rows[newrow->_y] = newrow;
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
  void transfer_alpha_typed(unsigned int width, unsigned char src_channels, const A* src_row, CMS::Format dest_format, const unsigned char* dest_row) {
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

  void transfer_alpha(unsigned int width, CMS::Format src_format, const unsigned char* src_row, CMS::Format dest_format, const unsigned char* dest_row) {
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

  void ImageRow::transform_colour(CMS::Transform::ptr transform, ImageRow::ptr dest_row) {
    transform->transform_buffer(_data, dest_row->_data, _image->width());
    if (dest_row->format().extra_channels())
      transfer_alpha(_image->width(), _image->format(), _data, dest_row->format(), dest_row->_data);
  }


  std::string profile_name(CMS::Profile::ptr profile) {
    return profile->description("en", "");
  }

  Image::ptr Image::transform_colour(CMS::Profile::ptr dest_profile, CMS::Format dest_format, CMS::Intent intent, bool can_free) {
    CMS::Profile::ptr profile = _profile;
    if (!_profile)
      profile = default_profile(_format, "source");
    if (!dest_profile)
      dest_profile = profile;

    CMS::Format orig_dest_format = dest_format;
    bool need_un_alpha_mult = false, need_alpha_mult = false;
    if (_format.extra_channels() > 0) {
      if (_format.is_premult_alpha() && (!dest_format.is_premult_alpha())) {
	need_un_alpha_mult = true;
	_format.unset_premult_alpha();
	_pixel_size = orig_dest_format.bytes_per_pixel();
	_row_size = _width * _pixel_size;
      } else if ((!_format.is_premult_alpha()) && dest_format.is_premult_alpha()) {
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

    auto transform = std::make_shared<CMS::Transform>(profile, _format,
						      dest_profile, dest_format,
						      intent, cmsFLAGS_NOCACHE);

    auto dest = std::make_shared<Image>(_width, _height, dest_format);
    dest->set_profile(dest_profile);
    dest->set_resolution(_xres, _yres);

    Timer timer;
    timer.start();

#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int y = 0; y < _height; y++) {
      dest->check_row_alloc(y);
      ImageRow::ptr src_row = row(y), dest_row = dest->row(y);

      if (need_un_alpha_mult) {
	auto temp = src_row->empty_copy();
	src_row->_un_alpha_mult(temp);
	src_row = temp;
	replace_row(temp);
      }

      src_row->transform_colour(transform, dest_row);

      if (need_alpha_mult) {
	auto temp = dest_row->empty_copy();
	dest_row->_alpha_mult(orig_dest_format, temp);
	dest_row = temp;
	dest->replace_row(temp);
      }

      if (can_free)
	this->free_row(y);

      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tTransformed " << y + 1 << " of " << _height << " rows";
    }
    timer.stop();
    std::cerr << "\r\tTransformed " << _height << " of " << _height << " rows." << std::endl;

    if (benchmark_mode) {
      std::cerr << std::setprecision(2) << std::fixed;
      long long pixel_count = _width * _height;
      std::cerr << "Benchmark: Transformed colourspace of " << pixel_count << " pixels in " << timer << " = " << (pixel_count / timer.elapsed() / 1e+6) << " Mpixels/second" << std::endl;
    }

    if (need_alpha_mult) {
      _format.set_channel_type(orig_dest_format);
      _format.set_premult_alpha();
      _format.set_packed();
      _pixel_size = orig_dest_format.bytes_per_pixel();
      _row_size = _width * _pixel_size;
    }

    return dest;
  }

  template <typename SRC>
  void ImageRow::_un_alpha_mult_src(ptr dest_row) {
    CMS::Format dest_format = format();
    SET_SAMPLE_FORMAT(dest_format);

    unsigned char alphachan = format().channels();
    SAMPLE scale = scaleval<SAMPLE>() / scaleval<SRC>();

    SRC *in = data<SRC>();
    SAMPLE *out = dest_row->data<SAMPLE>();
    for (unsigned int x = 0; x < width(); x++, in += format().total_channels(), out += dest_format.total_channels()) {
      SRC alpha = in[alphachan];
      unsigned char c;
      if (alpha > 0) {
	SAMPLE recip_alpha = scaleval<SAMPLE>() / alpha;
	for (c = 0; c < alphachan; c++)
	  out[c] = limitval<SAMPLE>(in[c] * recip_alpha);
      } else
	for (c = 0; c < alphachan; c++)
	  out[c] = 0; // or scaleval<SAMPLE>() ?
      for (; c < format().total_channels(); c++)
	out[c] = limitval<SAMPLE>(in[c] * scale);
    }
  }

  void ImageRow::_un_alpha_mult(ptr dest_row) {
    if ((format().extra_channels() > 0) && format().is_premult_alpha()) {
      if (format().is_8bit())
	_un_alpha_mult_src<unsigned char>(dest_row);
      else if (format().is_16bit())
	_un_alpha_mult_src<short unsigned int>(dest_row);
      else if (format().is_32bit())
	_un_alpha_mult_src<unsigned int>(dest_row);
      else if (format().is_float())
	_un_alpha_mult_src<float>(dest_row);
      else
	_un_alpha_mult_src<double>(dest_row);
    }
  }

  template <typename SRC, typename DST>
  void ImageRow::_alpha_mult_src_dst(CMS::Format dest_format, ptr dest_row) {
    unsigned char alphachan = format().channels();
    SAMPLE scale = (SAMPLE)scaleval<DST>() / scaleval<SRC>();
    SAMPLE src_scale = scale / scaleval<SRC>();

    SRC *in = data<SRC>();
    DST *out = dest_row->data<DST>();
    for (unsigned int x = 0; x < width(); x++, in += format().total_channels(), out += dest_format.total_channels()) {
      SAMPLE alpha = in[alphachan] * src_scale;
      unsigned char c;
      for (c = 0; c < alphachan; c++) {
	out[c] = limitval<DST>(in[c] * alpha);
      }
      for (; c < dest_format.total_channels(); c++)
	out[c] = limitval<DST>(in[c] * scale);
    }
  }

  template <typename SRC>
  void ImageRow::_alpha_mult_src(CMS::Format dest_format, ptr dest_row) {
    // We only take the channel type (bytes and float flag) and number of extra channels from dest_format
    {
      CMS::Format temp_format = format();
      temp_format.set_channel_type(dest_format);
      temp_format.set_extra_channels(dest_format.extra_channels());
      dest_format = temp_format;
    }

    if (dest_format.is_8bit())
      _alpha_mult_src_dst<SRC, unsigned char>(dest_format, dest_row);
    else if (dest_format.is_16bit())
      _alpha_mult_src_dst<SRC, short unsigned int>(dest_format, dest_row);
    else if (dest_format.is_32bit())
      _alpha_mult_src_dst<SRC, unsigned int>(dest_format, dest_row);
    else if (dest_format.is_float())
      _alpha_mult_src_dst<SRC, float>(dest_format, dest_row);
    else
      _alpha_mult_src_dst<SRC, double>(dest_format, dest_row);
  }

  void ImageRow::_alpha_mult(CMS::Format dest_format, ptr dest_row) {
    if ((format().extra_channels() > 0) && !format().is_premult_alpha()) {
      if (format().is_8bit())
	_alpha_mult_src<unsigned char>(dest_format, dest_row);
      else if (format().is_16bit())
	_alpha_mult_src<short unsigned int>(dest_format, dest_row);
      else if (format().is_32bit())
	_alpha_mult_src<unsigned int>(dest_format, dest_row);
      else if (format().is_float())
	_alpha_mult_src<float>(dest_format, dest_row);
      else
	_alpha_mult_src<double>(dest_format, dest_row);
    }
  }


} // namespace PhotoFinish
