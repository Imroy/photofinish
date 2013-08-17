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
#include <exiv2/exiv2.hpp>
#include "Definable.hh"
#include "CMS.hh"
#include "sample.h"

namespace PhotoFinish {

  //! An image class
  class Image {
  private:
    unsigned int _width, _height;
    CMS::Profile::ptr _profile;
    CMS::Format _format;
    size_t _pixel_size, _row_size;
    unsigned char **_rowdata;
    definable<double> _xres, _yres;		// PPI

    Exiv2::ExifData _EXIFtags;
    Exiv2::IptcData _IPTCtags;
    Exiv2::XmpData _XMPtags;

    template <typename SRC, typename DST>
    void _un_alpha_mult_src_dst(void);

    template <typename SRC, typename DST>
    void _alpha_mult_src_dst(CMS::Format dest_format);

    template <typename SRC>
    void _alpha_mult_src(CMS::Format dest_format);

  public:
    //! Shared pointer for an Image
    typedef std::shared_ptr<Image> ptr;

    //! Constructor
    /*!
      \param w,h Width and height of the image
      \param t LCMS2 pixel format
    */
    Image(unsigned int w, unsigned int h, CMS::Format f);

    //! Destructor
    ~Image();

    //! The width of this image
    inline const unsigned int width(void) const { return _width; }

    //! The height of this image
    inline const unsigned int height(void) const { return _height; }

    inline bool has_profile(void) const { return _profile == NULL ? false : true; }

    //! Get the ICC profile
    inline const CMS::Profile::ptr profile(void) const { return _profile; }

    //! Set the ICC profile
    inline void set_profile(CMS::Profile::ptr p) { _profile = p; }

    //! Get the CMS format
    inline CMS::Format format(void) const { return _format; }

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

    inline void check_rowdata_alloc(unsigned int y) {
      if (_rowdata[y] == NULL)
	_rowdata[y] = (unsigned char*)malloc(_row_size);
    }

    //! Pointer to pixel data at start of row
    template <typename T=void>
    inline T* row(unsigned int y) const { return (T*)_rowdata[y]; }

    //! Pointer to pixel data at coordinates
    template <typename T>
    inline T* at(unsigned int x, unsigned int y) const { return (T*)&_rowdata[y][x * _pixel_size]; }

    template <typename T>
    inline T& at(unsigned int x, unsigned int y, unsigned char c) const { return *((T*)&_rowdata[y][x * _pixel_size] + c); }

    //! Free the memory storing row 'y'
    inline void free_row(unsigned int y) {
      if (_rowdata[y] != NULL) {
	free(_rowdata[y]);
	_rowdata[y] = NULL;
      }
    }

    //! The Exiv2::ExifData object.
    inline Exiv2::ExifData& EXIFtags(void) { return _EXIFtags; }

    //! The Exiv2::IptcData object.
    inline Exiv2::IptcData& IPTCtags(void) { return _IPTCtags; }

    //! The Exiv2::XmpData object.
    inline Exiv2::XmpData& XMPtags(void) { return _XMPtags; }

    //! Create either an sRGB or greyscale profile depending on image format
    static CMS::Profile::ptr default_profile(CMS::ColourModel default_colourmodel, std::string for_desc);

    inline static CMS::Profile::ptr default_profile(CMS::Format format, std::string for_desc) { return default_profile(format.colour_model(), for_desc); }

    //! Transform this image into a different colour space and/or ICC profile, making a new image
    /*!
      \param dest_profile The ICC profile of the destination. If NULL, uses image's profile.
      \param dest_format The LCMS2 pixel format.
      \param intent The ICC intent of the transform, defaults to perceptual.
      \param can_free Whether rows can be freed after transforming, defaults to false.
      \return A new image
     */
    ptr transform_colour(CMS::Profile::ptr dest_profile, CMS::Format dest_format, CMS::Intent intent = CMS::Intent::Perceptual, bool can_free = false);

    //! Transform this image in-place into a different colour space and/or ICC profile
    /*!
      \param dest_profile The ICC profile of the destination. If NULL, uses image's profile.
      \param dest_format The LCMS2 pixel format.
      \param intent The ICC intent of the transform, defaults to perceptual.
     */
    void transform_colour_inplace(CMS::Profile::ptr dest_profile, CMS::Format dest_format, CMS::Intent intent = CMS::Intent::Perceptual);

    //! Un-pre-multiply the colour values with the alpha channel
    /*!
      Converts data to floating point (SAMPLE) in the process
     */
    void un_alpha_mult(void);

    //! Pre-multiply the colour values with the alpha
    /*!
      \param dest_format Destination format, only the channel type (bytes and float flag) are used.
     */
    void alpha_mult(CMS::Format dest_format);

  };

  //! A template function that we specialise for each type a pixel component could be in
  template <typename T>
  T maxval(void);

  template <>
  inline unsigned char maxval<unsigned char>(void) { return 255; }

  template <>
  inline unsigned short int maxval<unsigned short int>(void) { return 65535; }

  template <>
  inline unsigned int maxval<unsigned int>(void) { return 4294967295; }

  template <>
  inline float maxval<float>(void) { return 1.0; }

  template <>
  inline double maxval<double>(void) { return 1.0; }

}

#endif // __IMAGE_HH__
