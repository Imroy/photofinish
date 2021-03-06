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
#include <iomanip>
#include <omp.h>
#include "Frame.hh"
#include "Destination_items.hh"
#include "Kernel1Dvar.hh"

namespace PhotoFinish {

  Frame::Frame(double tw, double th, double x, double y, double w, double h) :
    D_target(std::string(""), tw, th),
    _crop_x(x), _crop_y(y),
    _crop_w(w), _crop_h(h)
  {}

  Frame::Frame(const D_target& target, double x, double y, double w, double h) :
    D_target(target),
    _crop_x(x), _crop_y(y),
    _crop_w(w), _crop_h(h)
  {}

  Image::ptr Frame::crop_resize(Image::ptr img, const D_resize& dr, bool can_free) {
    auto scale_width = Kernel1Dvar::create(dr, _crop_x, _crop_w, img->width(), _width);
    auto scale_height = Kernel1Dvar::create(dr, _crop_y, _crop_h, img->height(), _height);

    if (_width * img->height() < img->width() * _height) {
      auto temp = scale_width->convolve_h(img, can_free);
      return scale_height->convolve_v(temp, true);
    }

    auto temp = scale_height->convolve_v(img, can_free);
    return scale_width->convolve_h(temp, true);
  }

  const double Frame::waste(Image::ptr img) const {
    return ((img->width() - _crop_w) * _crop_h)
      + (img->width() * (img->height() - _crop_h));
  }

}
