/*
	Copyright 2014 Ian Tester

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
#include <vector>
#include <exiv2/exiv2.hpp>
#include "Definable.hh"
#include "CMS.hh"
#include "sample.h"

namespace PhotoFinish {

  class ImageRow;

  //! An image class
  class Image {
  private:
    unsigned int _width, _height;
    CMS::Profile::ptr _profile;
    CMS::Format _format;
    size_t _pixel_size, _row_size;
    std::vector<std::shared_ptr<ImageRow>> _rows;
    definable<double> _xres, _yres;		// PPI

    Exiv2::ExifData _EXIFtags;
    Exiv2::IptcData _IPTCtags;
    Exiv2::XmpData _XMPtags;

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

    inline void check_row_alloc(unsigned int y) {
      if (_rows[y] == nullptr)
	_rows[y] = std::make_shared<ImageRow>(this, y);
    }


    //! Row holder at a y value
    std::shared_ptr<ImageRow> row(unsigned int y) const { return _rows[y]; }

    //! Free the memory storing row 'y'
    inline void free_row(unsigned int y) {
      if (_rows[y] != NULL)
	_rows[y].reset();
    }

    void replace_row(std::shared_ptr<ImageRow> newrow);

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
      \return A new image
     */
    ptr transform_colour(CMS::Profile::ptr dest_profile, CMS::Format dest_format, CMS::Intent intent = CMS::Intent::Perceptual);

  };


  //! Class for holding a row of image data
  class ImageRow {
  private:
    const Image *_image;
    const unsigned int _y;
    unsigned char *_data;

    friend class Image;

    template <typename SRC>
    void _un_alpha_mult_src(std::shared_ptr<ImageRow> dest_row);

    template <typename SRC, typename DST>
    void _alpha_mult_src_dst(CMS::Format dest_format, std::shared_ptr<ImageRow> dest_row);

    template <typename SRC>
    void _alpha_mult_src(CMS::Format dest_format, std::shared_ptr<ImageRow> dest_row);

    //! Un-pre-multiply the colour values with the alpha channel
    /*!
      Converts data to floating point (SAMPLE) in the process
     */
    void _un_alpha_mult(std::shared_ptr<ImageRow> dest_row);

    //! Pre-multiply the colour values with the alpha
    /*!
      \param dest_format Destination format, only the channel type (bytes and float flag) are used.
     */
    void _alpha_mult(CMS::Format dest_format, std::shared_ptr<ImageRow> dest_row);

  public:
    typedef std::shared_ptr<ImageRow> ptr;

    //! Constructor
    ImageRow(const Image* img, unsigned int y) :
      _image(img),
      _y(y),
      _data(new unsigned char[_image->width() * _image->pixel_size()])
    {}

    ~ImageRow() {
      if (_data != nullptr)
	delete [] _data;
    }

    //! The width of the image
    inline const unsigned int width(void) const { return _image->width(); }

    //! The height of the image
    inline const unsigned int height(void) const { return _image->height(); }

    inline const unsigned int y(void) const { return _y; }

    //! Get the image ICC profile
    inline const CMS::Profile::ptr profile(void) const { return _image->profile(); }

    //! Get the CMS format
    inline CMS::Format format(void) const { return _image->format(); }

    //! The X resolution of the image (PPI)
    inline const definable<double> xres(void) const { return _image->xres(); }

    //! The Y resolution of the image (PPI)
    inline const definable<double> yres(void) const { return _image->yres(); }

    //! Make a copy pointing to the same image, same y value, etc, but don't copy the pixel values
    std::shared_ptr<ImageRow> empty_copy(void) const { return std::make_shared<ImageRow>(_image, _y); }

    template <typename T = unsigned char>
    inline T* data(unsigned int x = 0) const { return (T*)&_data[x * _image->pixel_size()]; }

    //! Transform this image row into a different colour space and/or ICC profile, making a new image
    void transform_colour(CMS::Transform::ptr transform, std::shared_ptr<ImageRow> dest_row);

  };


  //! A template function that returns the 'scale' value of a type.
  template <typename T>
  T scaleval(void);

  template <>
  inline unsigned char scaleval<unsigned char>(void) { return 0xff; }

  template <>
  inline unsigned short int scaleval<unsigned short int>(void) { return 0xffff; }

  template <>
  inline unsigned int scaleval<unsigned int>(void) { return 0xffffffff; }

  template <>
  inline unsigned long long scaleval<unsigned long long>(void) { return 0xffffffffffffffff; }

  template <>
  inline float scaleval<float>(void) { return 1.0; }

  template <>
  inline double scaleval<double>(void) { return 1.0; }


  //! A template function that limits a floating-point value while converting to another type.
  template <typename T>
  T limitval(SAMPLE v);

  template <>
  inline unsigned char limitval<unsigned char>(SAMPLE v) {
    if (v > 255.0)
      return 255;
    if (v < 0)
      return 0;
    return round(v);
  }

  template <>
  inline unsigned short int limitval<unsigned short int>(SAMPLE v) {
    if (v > 65535.0)
      return 65535;
    if (v < 0)
      return 0;
    return round(v);
  }

  template <>
  inline unsigned int limitval<unsigned int>(SAMPLE v) {
    if (v > 4294967295.0)
      return 4294967295;
    if (v < 0)
      return 0;
    return round(v);
  }

  template <>
  inline unsigned long long limitval<unsigned long long>(SAMPLE v) {
    if (v > 18446744073709551615.0)
      return 18446744073709551615UL;
    if (v < 0)
      return 0;
    return round(v);
  }

  template <>
  inline float limitval<float>(SAMPLE v) {
    return v;
  }

  template <>
  inline double limitval<double>(SAMPLE v) {
    return v;
  }

}

#endif // __IMAGE_HH__
