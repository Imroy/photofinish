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
#ifndef __KERNEL1DVAR_HH__
#define __KERNEL1DVAR_HH__

#include <memory>
#include <stdlib.h>
#include <math.h>
#include "Destination_items.hh"
#include "Exception.hh"
#include "sample.h"

namespace PhotoFinish {

  //! Creates and stores coefficients for cropping and resizing an image
  class Kernel1Dvar {
  protected:
    long int *_size, *_start;
    SAMPLE **_weights;
    double _to_size;
    long int _to_size_i;

    //! Private constructor
    Kernel1Dvar(double to_size);

    //! Build the kernel; used by derived classes
    void build(const D_resize& dr, double from_start, double from_size, long int from_max) throw(DestinationError);

    //! The size of this filter
    virtual double range(void) const = 0;

    //! Evaluate the filter at a given point
    virtual SAMPLE eval(double x) const throw(Uninitialised) = 0;

  public:
    typedef std::shared_ptr<Kernel1Dvar> ptr;

    //! Emoty constructor
    Kernel1Dvar();

    //! Named constructor
    /*! Create a Kernel1Dvar object using the filter name in the D_resize object.
      \param dr A D_resize object which will supply our parameters.
      \param from_start The starting point of the crop/resample
      \param from_size The size of the crop/resample
      \param from_max The size (maximum dimenstion) of the input
      \param to_size The size of the output
    */
    static ptr create(const D_resize& dr, double from_start, double from_size, long int from_max, double to_size) throw(DestinationError);

    //! Destructor
    ~Kernel1Dvar();

    //! Convolve an image horizontally with this kernel
    Image::ptr convolve_h(Image::ptr img);

    //! Convolve an image vertically with this kernel
    Image::ptr convolve_v(Image::ptr img);

    //! Accessor for the size of the weights
    inline const long int& size(long int i) const { return _size[i]; }

    //! Accessor for the starting point of the weights
    inline long int& start(long int i) const { return _start[i]; }

    //! Accessor for the weights array
    inline const SAMPLE* row(long int i) const { return _weights[i]; }

    //! Accessor for the weights array
    inline const SAMPLE& at(long int f, long int i) const { return _weights[i][f]; }

  };

  //! Lanczos filter
  class Lanczos : public Kernel1Dvar {
  private:
    bool _has_radius;
    double _radius, _r_radius;	//! Radius and its reciprocal

    double range(void) const;
    SAMPLE eval(double x) const throw(Uninitialised);

  public:
    //! Empty constructor
    Lanczos();

    //! Constructor
    /*!
      \param dr A D_resize object which will supply our parameters.
      \param horiz Will the kernel run in horizontal (true) or vertical direction?
      \param from_start The starting point of the crop/resample
      \param from_size The size of the crop/resample
      \param from_max The size (maximum dimenstion) of the input
      \param to_size The size of the output
    */
    Lanczos(const D_resize& dr, double from_start, double from_size, long int from_max, double to_size);
  };


}

#endif // __KERNEL1DVAR_HH__
