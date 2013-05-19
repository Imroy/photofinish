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
#include "Destination_items.hh"
#include "Exception.hh"
#include "Definable.hh"
#include "sample.h"

namespace PhotoFinish {

  //! Creates and stores coefficients for cropping and resizing an image
  class Kernel1Dvar {
  protected:
    unsigned int *_size, *_start;
    SAMPLE **_weights;
    double _scale, _to_size;
    unsigned int _to_size_i;

    //! Private constructor
    Kernel1Dvar(double to_size);

    //! Build the kernel; used by derived classes
    void build(double from_start, double from_size, unsigned int from_max) throw(DestinationError);

    //! The size of this filter
    virtual double range(void) const = 0;

    //! Evaluate the filter at a given point
    virtual SAMPLE eval(double x) const throw(Uninitialised) = 0;

    template <typename T>
    void do_convolve_h(Image::ptr src, Image::ptr dest, bool can_free = false);

    template <typename T>
    void do_convolve_v(Image::ptr src, Image::ptr dest, bool can_free = false);

  public:
    //! Shared pointer for a Kernel1Dvar
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
    static ptr create(const D_resize& dr, double from_start, double from_size, unsigned int from_max, double to_size) throw(DestinationError);

    //! Destructor
    ~Kernel1Dvar();

    //! Convolve an image horizontally with this kernel
    /*!
      \param img Source image
      \param can_free Can each row of the image be freed after it is convolved?
      \return New image
     */
    Image::ptr convolve_h(Image::ptr img, bool can_free = false);

    //! Convolve an image vertically with this kernel
    /*!
      \param img Source image
      \param can_free Can each row of the image be freed after it is convolved?
      \return New image
     */
    Image::ptr convolve_v(Image::ptr img, bool can_free = false);

  };

  //! Lanczos filter
  class Lanczos : public Kernel1Dvar {
  private:
    definable<double> _radius;	//! Radius
    double _r_radius;		//! Reciprocal of the radius

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
    Lanczos(const D_resize& dr, double from_start, double from_size, unsigned int from_max, double to_size);
  };


}

#endif // __KERNEL1DVAR_HH__
