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
#ifndef __IMAGE_HH__
#define __IMAGE_HH__

#include <memory>
#include <lcms2.h>
#include "Definable.hh"
#include "sample.h"

// Why doesn't lcms2.h define something like this?
#define T_BYTES_REAL(t) (T_BYTES(t) == 0 ? 8 : T_BYTES(t))

// Some masks for manipulating LCMS "types"
#define FLOAT_MASK	(0xffffffff ^ FLOAT_SH(1))
#define OPTIMIZED_MASK	(0xffffffff ^ OPTIMIZED_SH(1))
#define COLORSPACE_MASK	(0xffffffff ^ COLORSPACE_SH(31))
#define SWAPFIRST_MASK	(0xffffffff ^ SWAPFIRST_SH(1))
#define FLAVOR_MASK	(0xffffffff ^ FLAVOR_SH(1))
#define PLANAR_MASK	(0xffffffff ^ PLANAR_SH(1))
#define ENDIAN16_MASK	(0xffffffff ^ ENDIAN_SH(1))
#define DOSWAP_MASK	(0xffffffff ^ DOSWAP_SH(1))
#define EXTRA_MASK	(0xffffffff ^ EXTRA_SH(7))
#define CHANNELS_MASK	(0xffffffff ^ CHANNELS_SH(15))
#define BYTES_MASK	(0xffffffff ^ BYTES_SH(7))

namespace PhotoFinish {

  //! An image class
  class Image {
  private:
    unsigned int _width, _height;
    cmsHPROFILE _profile;
    cmsUInt32Number _type;
    size_t _pixel_size, _row_size;
    unsigned char **_rowdata;
    definable<double> _xres, _yres;		// PPI

    inline void _check_rowdata_alloc(unsigned int y) {
      if (_rowdata[y] == NULL)
	_rowdata[y] = (unsigned char*)malloc(_row_size);
    }

  public:
    //! Shared pointer for an Image
    typedef std::shared_ptr<Image> ptr;

    //! Constructor
    /*!
      \param w,h Width and height of the image
      \param t LCMS2 pixel type
    */
    Image(unsigned int w, unsigned int h, cmsUInt32Number t);

    //! Destructor
    ~Image();

    //! The width of this image
    inline const unsigned int width(void) const { return _width; }

    //! The height of this image
    inline const unsigned int height(void) const { return _height; }

    //! Get the ICC profile
    inline const cmsHPROFILE profile(void) const { return _profile; }

    //! Set the ICC profile
    inline void set_profile(cmsHPROFILE p) { _profile = p; }

    //! Get the CMS type
    inline cmsUInt32Number type(void) const { return _type; }

    //! The X resolution of this image (PPI)
    inline const definable<double> xres(void) const { return _xres; }

    //! The Y resolution of this image (PPI)
    inline const definable<double> yres(void) const { return _yres; }

    //! Set both the X and Y resolution (PPI)
    inline void set_resolution(double r) { _xres = _yres = r; }

    //! Set the X resolution (PPI)
    inline void set_xres(double r) { _xres = r; }

    //! Set the Y resolution (PPI)
    inline void set_yres(double r) { _yres = r; }

    //! Set the X and Y resolutions (PPI)
    inline void set_resolution(double xr, double yr) { _xres = xr; _yres = yr; }

    //! Set the resolution given the length of the longest side (in inches)
    inline void set_resolution_from_size(double size) { _xres = _yres = (_width > _height ? _width : _height) / size; }

    //! Return the size of a pixel in bytes
    inline size_t pixel_size(void) const { return _pixel_size; }

    //! Retun the size of a row in bytes
    inline size_t row_size(void) const { return _row_size; }

    //! Pointer to pixel data at start of row
    inline unsigned char* row(unsigned int y) {
      _check_rowdata_alloc(y);
      return _rowdata[y];
    }

    //! Pointer to pixel data at coordinates
    inline unsigned char* at(unsigned int x, unsigned int y) {
      _check_rowdata_alloc(y);
      return &(_rowdata[y][x * _pixel_size]);
    }

    //! Free the memory storing row 'y'
    inline void free_row(unsigned int y) {
      if (_rowdata[y] != NULL) {
	free(_rowdata[y]);
	_rowdata[y] = NULL;
      }
    }

    //! Create either an sRGB or greyscale profile depending on image type
    static cmsHPROFILE default_profile(cmsUInt32Number default_type);

    //! Transform this image into a different colour space and/or ICC profile, making a new image
    /*!
      \param dest_profile The ICC profile of the destination
      \param dest_type The LCMS2 pixel format
      \param intent The ICC intent of the transform, defaults to perceptual
      \param can_free Whether rows can be freed after transforming, defaults to false.
      \return A new image
     */
    ptr transform_colour(cmsHPROFILE dest_profile, cmsUInt32Number dest_type, cmsUInt32Number intent = INTENT_PERCEPTUAL, bool can_free = false);

    //! Transform this image in-place into a different colour space and/or ICC profile
    /*!
      \param dest_profile The ICC profile of the destination
      \param dest_type The LCMS2 pixel format
      \param intent The ICC intent of the transform, defaults to perceptual
     */
    void transform_colour_inplace(cmsHPROFILE dest_profile, cmsUInt32Number dest_type, cmsUInt32Number intent = INTENT_PERCEPTUAL);

  };

}

#endif // __IMAGE_HH__
