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
#ifndef __FRAME_HH__
#define __FRAME_HH__

#include "Destination_items.hh"
#include "Filter.hh"

namespace PhotoFinish {

  class Frame : public D_target {
  private:
    // width and height attributes inherited from D_target are the final size of the image
    double _crop_x, _crop_y, _crop_w, _crop_h;	// coordinates for cropping from the original image
    double _resolution;				// PPI

  public:
    Frame(const D_target& target, double x, double y, double w, double h, double r);
    ~Frame();

    // Resize the image using a Lanczos filter
    const Image& crop_resize(const Image& img, const _Filter* filter);

    inline double crop_x(void) const { return _crop_x; }
    inline double crop_y(void) const { return _crop_y; }
    inline double crop_w(void) const { return _crop_w; }
    inline double crop_h(void) const { return _crop_h; }
    inline double resolution(void) const { return _resolution; }
  };

}

#endif /* __FRAME_HH__ */
