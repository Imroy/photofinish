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

  Image::Image(unsigned int w, unsigned int h, cmsUInt32Number t) :
    _width(w),
    _height(h),
    _profile(NULL),
    _type(t),
    _row_size(0),
    _rowdata(NULL)
  {
    unsigned char bytes = T_BYTES_REAL(_type);
    _pixel_size = (T_CHANNELS(_type) + T_EXTRA(_type)) * bytes;
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
    free(_profile);
  }

  cmsHPROFILE Image::default_profile(cmsUInt32Number default_type) {
    cmsHPROFILE profile = NULL;
    switch (T_COLORSPACE(default_type)) {
    case PT_RGB:
      std::cerr << "\tUsing default sRGB profile." << std::endl;
      profile = cmsCreate_sRGBProfile();
      break;

    case PT_GRAY:
      std::cerr << "\tUsing default greyscale profile." << std::endl;
      // Build a greyscale profile with the same gamma curve and white point as sRGB.
      // Copied from cmsCreate_sRGBProfileTHR() and Build_sRGBGamma().
      {
	cmsCIExyY D65;
	cmsWhitePointFromTemp(&D65, 6504);
	double Parameters[5] = {
	  2.4,			// Gamma
	  1.0 / 1.055,		// a
	  0.055 / 1.055,	// b
	  1.0 / 12.92,		// c
	  0.04045,		// d
	};
	// y = (x >= d ? (a*x + b)^Gamma : c*x)
	cmsToneCurve *gamma = cmsBuildParametricToneCurve(NULL, 4, Parameters);
	profile = cmsCreateGrayProfile(&D65, gamma);
	cmsFreeToneCurve(gamma);

	cmsMLU *DescriptionMLU = cmsMLUalloc(NULL, 1);
	if (DescriptionMLU != NULL) {
	  if (cmsMLUsetWide(DescriptionMLU,  "en", "AU", L"sGrey built-in"))
	    cmsWriteTag(profile, cmsSigProfileDescriptionTag,  DescriptionMLU);
	  cmsMLUfree(DescriptionMLU);
	}
      }
      break;

    case PT_Lab:
      std::cerr << "\tUsing default Lab profile." << std::endl;
      profile = cmsCreateLab4Profile(NULL);
      break;

    default:
      std::cerr << "** Cannot assign a default profile for colour space " << T_COLORSPACE(default_type) << " **" << std::endl;
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

  void transfer_alpha(unsigned int width, cmsUInt32Number src_type, const unsigned char* src_row, cmsUInt32Number dest_type, const unsigned char* dest_row) {
    unsigned char src_channels = (unsigned char)T_CHANNELS(src_type);
    unsigned char dest_channels = (unsigned char)T_CHANNELS(dest_type);
    switch (T_BYTES_REAL(src_type)) {
    case 1:
      switch (T_BYTES_REAL(dest_type)) {
      case 1:
	do_transfer_alpha<unsigned char, unsigned char>(width, src_channels,src_row, dest_channels,dest_row);
	break;

      case 2:
	do_transfer_alpha<unsigned char, short unsigned int>(width, src_channels,src_row, dest_channels,(short unsigned int*)dest_row);
	break;

      case 4:
	if (T_FLOAT(dest_type))
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
      switch (T_BYTES_REAL(dest_type)) {
      case 1:
	do_transfer_alpha<short unsigned int, unsigned char>(width, src_channels,(short unsigned int*)src_row, dest_channels,dest_row);
	break;

      case 2:
	do_transfer_alpha<short unsigned int, short unsigned int>(width, src_channels,(short unsigned int*)src_row, dest_channels,(short unsigned int*)dest_row);
	break;

      case 4:
	if (T_FLOAT(dest_type))
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
      if (T_FLOAT(src_type)) {
	switch (T_BYTES_REAL(dest_type)) {
	case 1:
	  do_transfer_alpha<float, unsigned char>(width, src_channels,(float*)src_row, dest_channels,dest_row);
	  break;

	case 2:
	  do_transfer_alpha<float, short unsigned int>(width, src_channels,(float*)src_row, dest_channels,(short unsigned int*)dest_row);
	  break;

	case 4:
	  if (T_FLOAT(dest_type))
	    do_transfer_alpha<float, float>(width, src_channels,(float*)src_row, dest_channels,(float*)dest_row);
	  else
	    do_transfer_alpha<float, unsigned int>(width, src_channels,(float*)src_row, dest_channels,(unsigned int*)dest_row);
	  break;

	case 8:
	  do_transfer_alpha<float, double>(width, src_channels,(float*)src_row, dest_channels,(double*)dest_row);
	  break;

	}
      } else {
	switch (T_BYTES_REAL(dest_type)) {
	case 1:
	  do_transfer_alpha<unsigned int, unsigned char>(width, src_channels,(unsigned int*)src_row, dest_channels,dest_row);
	  break;

	case 2:
	  do_transfer_alpha<unsigned int, short unsigned int>(width, src_channels,(unsigned int*)src_row, dest_channels,(short unsigned int*)dest_row);
	  break;

	case 4:
	  if (T_FLOAT(dest_type))
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
      switch (T_BYTES_REAL(dest_type)) {
      case 1:
	do_transfer_alpha<double, unsigned char>(width, src_channels,(double*)src_row, dest_channels,dest_row);
	break;

      case 2:
	do_transfer_alpha<double, short unsigned int>(width, src_channels,(double*)src_row, dest_channels,(short unsigned int*)dest_row);
	break;

      case 4:
	if (T_FLOAT(dest_type))
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

  Image::ptr Image::transform_colour(cmsHPROFILE dest_profile, cmsUInt32Number dest_type, cmsUInt32Number intent, bool can_free) {
#pragma omp parallel
    {
#pragma omp master
      {
	std::cerr << "Transforming colour using " << omp_get_num_threads() << " threads..." << std::endl;
      }
    }

    cmsHPROFILE profile = _profile;
    bool own_profile = false;
    if (_profile == NULL) {
      profile = default_profile(_type);
      own_profile = true;
      if (dest_profile == NULL) {
	dest_profile = profile;
	own_profile = false;
      }
    } else
      if (dest_profile == NULL)
	dest_profile = default_profile(dest_type);

    cmsHTRANSFORM transform = cmsCreateTransform(profile, _type,
						 dest_profile, dest_type,
						 intent, 0);
    if (own_profile)
      cmsCloseProfile(profile);

    auto dest = std::make_shared<Image>(_width, _height, dest_type);
    dest->set_profile(dest_profile);
    dest->set_resolution(_xres, _yres);

#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int y = 0; y < _height; y++) {
      cmsDoTransform(transform, _rowdata[y], dest->row(y), _width);
      if (T_EXTRA(dest_type))
	transfer_alpha(_width, _type, _rowdata[y], dest_type, dest->row(y));

      if (can_free)
	this->free_row(y);
      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tTransformed " << y + 1 << " of " << _height << " rows";
    }
    std::cerr << "\r\tTransformed " << _height << " of " << _height << " rows." << std::endl;

    cmsDeleteTransform(transform);

    return dest;
  }

  void Image::transform_colour_inplace(cmsHPROFILE dest_profile, cmsUInt32Number dest_type, cmsUInt32Number intent) {
#pragma omp parallel
    {
#pragma omp master
      {
	std::cerr << "Transforming colour in-place using " << omp_get_num_threads() << " threads..." << std::endl;
      }
    }

    cmsHPROFILE profile = _profile;
    bool own_profile = false;
    if (_profile == NULL) {
      profile = default_profile(_type);
      own_profile = true;
      if (dest_profile == NULL) {
	dest_profile = profile;
	own_profile = false;
      }
    } else
      if (dest_profile == NULL)
	dest_profile = default_profile(dest_type);

    cmsHTRANSFORM transform = cmsCreateTransform(profile, _type,
						 dest_profile, dest_type,
						 intent, 0);
    if (own_profile)
      cmsCloseProfile(profile);

    unsigned char dest_bytes = T_BYTES_REAL(dest_type);
    size_t dest_pixel_size = (T_CHANNELS(dest_type) + T_EXTRA(dest_type)) * dest_bytes;
    size_t dest_row_size = _width * dest_pixel_size;

#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int y = 0; y < _height; y++) {
      _check_rowdata_alloc(y);
      unsigned char *src_rowdata = _rowdata[y];
      unsigned char *dest_rowdata = (unsigned char*)malloc(dest_row_size);
      cmsDoTransform(transform, src_rowdata, dest_rowdata, _width);
      if (T_EXTRA(dest_type))
	transfer_alpha(_width, _type, src_rowdata, dest_type, dest_rowdata);

      _rowdata[y] = dest_rowdata;
      free(src_rowdata);

      if (omp_get_thread_num() == 0)
	std::cerr << "\r\tTransformed " << y + 1 << " of " << _height << " rows";
    }
    std::cerr << "\r\tTransformed " << _height << " of " << _height << " rows." << std::endl;

    cmsDeleteTransform(transform);

    _profile = dest_profile;
    _type = dest_type;
    _pixel_size = dest_pixel_size;
    _row_size = dest_row_size;
  }

}
