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

#include <memory>
#include "Destination_items.hh"
#include "Filter.hh"

namespace PhotoFinish {

  //! Crop+rescaling parameters
  class Frame : public D_target {
  private:
    double _crop_x, _crop_y, _crop_w, _crop_h;	// coordinates for cropping from the original image
    double _resolution;				// PPI

  public:
    //! Constructor
    /*!
      \param tw,th Size (width, height) of the output
      \param x,y Top-left corner of crop+rescale window
      \param w,h Size of the crop+rescale window
      \param r Resolution (PPI)
    */
    Frame(double tw, double th, double x, double y, double w, double h, double r);

    //! Constructor
    /*!
      \param target D_target object providing the size (width, height) of the output
      \param x,y Top-left corner of crop+rescale window
      \param w,h Size of the crop+rescale window
      \param r Resolution (PPI)
    */
    Frame(const D_target& target, double x, double y, double w, double h, double r);

    //! Crop and resize an image
    /*!
      \param img The source image
      \param filter The filter to use
      \return A new cropped and resized image
    */
    Image::ptr crop_resize(Image::ptr img, Filter::ptr filter);

    //! Accessor
    inline const double crop_x(void) const { return _crop_x; }
    //! Accessor
    inline const double crop_y(void) const { return _crop_y; }
    //! Accessor
    inline const double crop_w(void) const { return _crop_w; }
    //! Accessor
    inline const double crop_h(void) const { return _crop_h; }
    //! Accessor
    inline const double resolution(void) const { return _resolution; }

    typedef std::shared_ptr<Frame> ptr;
  };

}

#endif /* __FRAME_HH__ */
