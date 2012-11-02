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
#ifndef __RESCALER_HH__
#define __RESCALER_HH__

#include <math.h>
#include "Function1D.hh"
#include "Destination.hh"
#include "Image.hh"
#include "Exception.hh"

#define sqr(x) ((x) * (x))

namespace PhotoFinish {

  //! Lanczos filter
  class Lanczos : public Function1D {
  private:
    double _radius;	//! Radius
    double _r_radius;	//! Reciprocal of the radius

  public:
    //! Empty constructor
    /*!
      Sets default value of radius = 3.0
    */
    Lanczos();

    //! Constructor
    Lanczos(const D_resize& dr);

    inline double range(void) const { return _radius; }

    inline double eval(double x) const {
      if (fabs(x) < 1e-6)
	return 1.0;
      double pix = M_PI * x;
      return (_radius * sin(pix) * sin(pix * _r_radius)) / sqr(pix);
    }
  }; //class Lanczos



  //! Abstract base class for that creates and stores coefficients for cropping and resizing an image
  class Rescaler : public ImageSink, public ImageSource {
  protected:
    Function1D::ptr _func;
    unsigned int *_size, *_start;
    SAMPLE **_weights;
    double _scale, _to_size;
    unsigned int _to_size_i;
    ImageHeader::ptr _rescaled_header;

    virtual void _work_on_row(ImageRow::ptr row) = 0;

  public:
    //! Constructor
    Rescaler(Function1D::ptr func, double from_start, double from_size, unsigned int from_max, double to_size);

    //! Destructor
    ~Rescaler();

    virtual void receive_image_header(ImageHeader::ptr header) = 0;
  };



  //! Crop and/or rescale in the horizontal direction
  class Rescaler_width : public Rescaler {
  protected:
    virtual void _work_on_row(ImageRow::ptr row);

    template <typename P>
    void _scale_row(ImageRow::ptr row);

  public:
    //! Constructor
    Rescaler_width(Function1D::ptr func, double from_start, double from_size, unsigned int from_max, double to_size);

    virtual void receive_image_header(ImageHeader::ptr header);

  }; // class Rescaler_width



  //! Crop and/or rescale in the vertical direction
  class Rescaler_height : public Rescaler {
  protected:
    unsigned int *_row_counts;
    ImageRow::ptr *_rows;

    virtual void _work_on_row(ImageRow::ptr row);

    template <typename P>
    void _scale_row(ImageRow::ptr row);

  public:
    //! Constructor
    Rescaler_height(Function1D::ptr func, double from_start, double from_size, unsigned int from_max, double to_size);

    ~Rescaler_height();

    virtual void receive_image_header(ImageHeader::ptr header);
    virtual void receive_image_row(ImageRow::ptr row);

  }; // class Rescaler_height



  //! Abstract base class for ImageModifier that rescales images
  class mod_BaseRescaler : public ImageModifier {
  protected:
    ImageSink::ptr _sink;

    void _add_rescalers(ImageHeader::ptr header, Function1D::ptr func, double crop_x, double crop_y, double crop_w, double crop_h, double new_width, double new_height);

  public:
    //! Constructor
    /*!
      \param source The Image source to attach to
      \param sink The Image sink that rescaled images will be sent to
      \param workgang The Workgang that the rescalers will be added to.
     */
    mod_BaseRescaler(ImageSource::ptr source, ImageSink::ptr sink, WorkGang::ptr workgang);

    virtual void receive_image_header(ImageHeader::ptr header) = 0;
  }; // class mod_BaseRescaler



  //! Image modifier that scales images by a fixed factor
  class mod_FixedFactorRescaler : public mod_BaseRescaler {
  protected:
    double _factor;

  public:
    //! Constructor
    /*!
      \param factor The fixed factor that the image will be scaled by.
     */
    mod_FixedFactorRescaler(ImageSource::ptr source, ImageSink::ptr sink, WorkGang::ptr workgang, double factor);

    virtual void receive_image_header(ImageHeader::ptr header);

  }; // class mod_FixedFactorRescaler



  //! Image modifier that scales to a fixed size
  class mod_FixedSizeRescaler : public mod_BaseRescaler {
  protected:
    double _width, _height;
    bool _upscale, _inside;

  public:
    //! Constructor
    /*!
      \param width,height The size that the image will be rescaled to
      \param upscale Do we allow the image to be made bigger?
      \param inside Should we scale the image to fit totally inside the provided width×height?
     */
    mod_FixedSizeRescaler(ImageSource::ptr source, ImageSink::ptr sink, WorkGang::ptr workgang, double width, double height, bool upscale = false, bool inside = true);

    virtual void receive_image_header(ImageHeader::ptr header);

  }; // class mod_FixedSizeRescaler



  //! Image modifier that scales to fit one of the targets of a Destination
  class mod_DestinationRescaler : public mod_BaseRescaler {
  protected:
    Destination::ptr _dest;

  public:
    mod_DestinationRescaler(ImageSource::ptr source, ImageSink::ptr sink, WorkGang::ptr workgang, Destination::ptr dest);

    virtual void receive_image_header(ImageHeader::ptr header);

  }; // class mod_DestinationRescaler

} // namespace PhotoFinish

#endif // __SCALER_HH__
