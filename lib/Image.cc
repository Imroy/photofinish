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
  void do_transfer_alpha(unsigned int width, unsigned char src_channels, const A* src_row, unsigned char dest_channels, const B* dest_row) {
    double factor = (double)maxval<B>() / maxval<A>();
    A *inp = const_cast<A*>(src_row) + src_channels;
    B *outp = const_cast<B*>(dest_row) + dest_channels;
    for (unsigned int x = 0; x < width; x++, inp += 1 + src_channels, outp += 1 + dest_channels) {
      if (*inp < 0)
	*outp = 0;
      else if (*inp > maxval<A>())
	*outp = maxval<B>();
      else
	*outp = (*inp) * factor;
    }
  }

  void transfer_alpha(unsigned int width, CMS::Format src_format, const unsigned char* src_row, CMS::Format dest_format, const unsigned char* dest_row) {
    unsigned char src_channels = (unsigned char)src_format.channels();
    unsigned char dest_channels = (unsigned char)dest_format.channels();
    switch (src_format.bytes_per_channel()) {
    case 1:
      switch (dest_format.bytes_per_channel()) {
      case 1:
	do_transfer_alpha<unsigned char, unsigned char>(width, src_channels,src_row, dest_channels,dest_row);
	break;

      case 2:
	do_transfer_alpha<unsigned char, short unsigned int>(width, src_channels,src_row, dest_channels,(short unsigned int*)dest_row);
	break;

      case 4:
	if (dest_format.is_fp())
	  do_transfer_alpha<unsigned char, float>(width, src_channels,src_row, dest_channels,(float*)dest_row);
	else
	  do_transfer_alpha<unsigned char, unsigned int>(width, src_channels,src_row, dest_channels,(unsigned int*)dest_row);
	break;

      case 8:
	do_transfer_alpha<unsigned char, double>(width, src_channels,src_row, dest_channels,(double*)dest_row);
	break;

      }
      break;

    case 2:
      switch (dest_format.bytes_per_channel()) {
      case 1:
	do_transfer_alpha<short unsigned int, unsigned char>(width, src_channels,(short unsigned int*)src_row, dest_channels,dest_row);
	break;

      case 2:
	do_transfer_alpha<short unsigned int, short unsigned int>(width, src_channels,(short unsigned int*)src_row, dest_channels,(short unsigned int*)dest_row);
	break;

      case 4:
	if (dest_format.is_fp())
	  do_transfer_alpha<short unsigned int, float>(width, src_channels,(short unsigned int*)src_row, dest_channels,(float*)dest_row);
	else
	  do_transfer_alpha<short unsigned int, unsigned int>(width, src_channels,(short unsigned int*)src_row, dest_channels,(unsigned int*)dest_row);
	break;

      case 8:
	do_transfer_alpha<short unsigned int, double>(width, src_channels,(short unsigned int*)src_row, dest_channels,(double*)dest_row);
	break;

      }
      break;

    case 4:
      if (src_format.is_fp()) {
	switch (dest_format.bytes_per_channel()) {
	case 1:
	  do_transfer_alpha<float, unsigned char>(width, src_channels,(float*)src_row, dest_channels,dest_row);
	  break;

	case 2:
	  do_transfer_alpha<float, short unsigned int>(width, src_channels,(float*)src_row, dest_channels,(short unsigned int*)dest_row);
	  break;

	case 4:
	  if (dest_format.is_fp())
	    do_transfer_alpha<float, float>(width, src_channels,(float*)src_row, dest_channels,(float*)dest_row);
	  else
	    do_transfer_alpha<float, unsigned int>(width, src_channels,(float*)src_row, dest_channels,(unsigned int*)dest_row);
	  break;

	case 8:
	  do_transfer_alpha<float, double>(width, src_channels,(float*)src_row, dest_channels,(double*)dest_row);
	  break;

	}
      } else {
	switch (dest_format.bytes_per_channel()) {
	case 1:
	  do_transfer_alpha<unsigned int, unsigned char>(width, src_channels,(unsigned int*)src_row, dest_channels,dest_row);
	  break;

	case 2:
	  do_transfer_alpha<unsigned int, short unsigned int>(width, src_channels,(unsigned int*)src_row, dest_channels,(short unsigned int*)dest_row);
	  break;

	case 4:
	  if (dest_format.is_fp())
	    do_transfer_alpha<unsigned int, float>(width, src_channels,(unsigned int*)src_row, dest_channels,(float*)dest_row);
	  else
	    do_transfer_alpha<unsigned int, unsigned int>(width, src_channels,(unsigned int*)src_row, dest_channels,(unsigned int*)dest_row);
	  break;

	case 8:
	  do_transfer_alpha<unsigned int, double>(width, src_channels,(unsigned int*)src_row, dest_channels,(double*)dest_row);
	  break;

	}
      }
      break;

    case 8:
      switch (dest_format.bytes_per_channel()) {
      case 1:
	do_transfer_alpha<double, unsigned char>(width, src_channels,(double*)src_row, dest_channels,dest_row);
	break;

      case 2:
	do_transfer_alpha<double, short unsigned int>(width, src_channels,(double*)src_row, dest_channels,(short unsigned int*)dest_row);
	break;

      case 4:
	if (dest_format.is_fp())
	  do_transfer_alpha<double, float>(width, src_channels,(double*)src_row, dest_channels,(float*)dest_row);
	else
	  do_transfer_alpha<double, unsigned int>(width, src_channels,(double*)src_row, dest_channels,(unsigned int*)dest_row);
	break;

      case 8:
	do_transfer_alpha<double, double>(width, src_channels,(double*)src_row, dest_channels,(double*)dest_row);
	break;

      }
      break;

    }

  }

  std::string profile_name(CMS::Profile::ptr profile) {
    return profile->read_info(cmsInfoDescription, "en", cmsNoCountry);
  }

  Image::ptr Image::transform_colour(CMS::Profile::ptr dest_profile, CMS::Format dest_format, CMS::Intent intent, bool can_free) {
    CMS::Profile::ptr profile = _profile;
    if (_profile == NULL)
      profile = default_profile(_format, "source");
    if (dest_profile == NULL)
      dest_profile = profile;

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
      transform.transform_buffer(_rowdata[y], dest->row(y), _width);
      if (dest_format.extra_channels())
	transfer_alpha(_width, _format, _rowdata[y], dest_format, dest->row(y));

      if (can_free)
	this->free_row(y);
      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tTransformed " << y + 1 << " of " << _height << " rows";
    }
    std::cerr << "\r\tTransformed " << _height << " of " << _height << " rows." << std::endl;

    return dest;
  }

  void Image::transform_colour_inplace(CMS::Profile::ptr dest_profile, CMS::Format dest_format, CMS::Intent intent) {
    CMS::Profile::ptr profile = _profile;
    if (_profile == NULL)
      profile = default_profile(_format, "source");
    if (dest_profile == NULL)
      dest_profile = profile;

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
      _check_rowdata_alloc(y);
      unsigned char *src_rowdata = _rowdata[y];
      unsigned char *dest_rowdata = (unsigned char*)malloc(dest_row_size);
      transform.transform_buffer(src_rowdata, dest_rowdata, _width);
      if (dest_format.extra_channels())
	transfer_alpha(_width, _format, src_rowdata, dest_format, dest_rowdata);

      _rowdata[y] = dest_rowdata;
      free(src_rowdata);

      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tTransformed " << y + 1 << " of " << _height << " rows";
    }
    std::cerr << "\r\tTransformed " << _height << " of " << _height << " rows." << std::endl;

    _profile = dest_profile;
    _format = dest_format;
    _pixel_size = dest_pixel_size;
    _row_size = dest_row_size;
  }

}
