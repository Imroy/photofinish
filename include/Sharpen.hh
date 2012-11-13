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
#ifndef __SHARPEN_HH__
#define __SHARPEN_HH__

#include <memory>
#include "Function2D.hh"
#include "Image.hh"
#include "Destination_items.hh"
#include "Exception.hh"
#include "sample.h"

#define sqr(x) ((x) * (x))

namespace PhotoFinish {

  //! Gaussian 2D function
  class Gaussian : public Function2D {
  private:
    double _radius, _sigma;	// Radius
    double _safe_sigma_sqr;

  public:
    //! Empty constructor
    Gaussian();

    //! Constructor
    /*!
      \param ds A D_sharpen object which will supply our parameters.
    */
    Gaussian(const D_sharpen& ds);

    inline double range(void) const { return _radius; }

    inline double eval(double x, double y) const { return -exp((sqr(x) + sqr(y)) / (-2.0 * _safe_sigma_sqr)); }
  };



  //! Creates and stores coefficients for convolving an image
  class Sharpen : public ImageSink, public ImageSource {
  protected:
    short unsigned int _width, _height, _centrex, _centrey;
    SAMPLE **_values;
    ImageHeader::ptr _header;
    unsigned int *_row_counts;
    ImageRow::ptr *_rows;

    virtual void _work_on_row(ImageRow::ptr row);

    inline SAMPLE* _row(short unsigned int y) const { return _values[y]; }
    inline SAMPLE& _at(short unsigned int x, short unsigned int y) const { return _values[y][x]; }

    template <typename P>
    void _sharpen_row(ImageRow::ptr row);

  public:
    //! Constructor
    Sharpen(Function2D::ptr func);

    //! Destructor
    ~Sharpen();

    virtual void receive_image_header(ImageHeader::ptr header);
  };

}

#endif // __SHARPEN_HH__
