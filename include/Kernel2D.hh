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
#ifndef __KERNEL_HH__
#define __KERNEL_HH__

#include <memory>
#include "Image.hh"
#include "Exception.hh"
#include "Definable.hh"
#include "sample.h"

namespace PhotoFinish {

  class D_sharpen;

  //! Creates and stores coefficients for convolving an image
  class Kernel2D {
  protected:
    short unsigned int _width, _height, _centrex, _centrey;
    SAMPLE **_values;

    //! Private constructor for derived classes
    Kernel2D(short unsigned int w, short unsigned int h, short unsigned int cx, short unsigned int cy);

    //! Private constructor for square filters
    Kernel2D(short unsigned int size, short unsigned int centre);

  public:
    typedef std::shared_ptr<Kernel2D> ptr;

    //! Empty constructor
    Kernel2D();

    //! Named constructor
    /*! Create a Kernel2D object using the parameters in the D_sharpen object.
      \param ds A D_sharpen object which will supply our parameters.
    */
    static ptr create(const D_sharpen& ds) throw(DestinationError);

    //! Destructor
    ~Kernel2D();

    inline const short unsigned int width(void) const { return _width; }
    inline const short unsigned int height(void) const { return _height; }
    inline const short unsigned int centrex(void) const { return _centrex; }
    inline const short unsigned int centrey(void) const { return _centrey; }

    //! Accessor for the values array
    inline SAMPLE* row(short unsigned int y) const { return _values[y]; }

    //! Accessor for the values array
    inline SAMPLE& at(short unsigned int x, short unsigned int y) const { return _values[y][x]; }

    //! Convolve and image with this kernel and produce a new image
    Image::ptr convolve(Image::ptr img);
  };

  //! GaussianSharpen kernel
  class GaussianSharpen : public Kernel2D {
  private:
    definable<double> _radius, _sigma;	// Radius
    double _safe_sigma_sqr;

  public:
    //! Empty constructor
    GaussianSharpen();

    //! Constructor
    /*!
      \param ds A D_sharpen object which will supply our parameters.
    */
    GaussianSharpen(const D_sharpen& ds);
  };



}

#endif // __KERNEL_HH__
