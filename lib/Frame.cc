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
#include <iomanip>
#include <omp.h>
#include "Frame.hh"
#include "Destination_items.hh"

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

  const double Frame::waste(ImageHeader::ptr header) const {
    return ((header->width() - _crop_w) * _crop_h)
      + (header->width() * (header->height() - _crop_h));
  }

}
