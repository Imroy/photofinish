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
#ifndef __IMAGEFILE_HH__
#define __IMAGEFILE_HH__

#include <string>
#include <memory>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <lcms2.h>

#ifdef HAZ_PNG
#include <png.h>
#endif
#ifdef HAZ_JPEG
#include <jpeglib.h>
#endif
#ifdef HAZ_TIFF
#include <tiffio.h>
#endif
#ifdef HAZ_JP2
#include <openjpeg.h>
#endif

#include "Image.hh"
#include "Destination.hh"
#include "Exception.hh"
#include "sample.h"

#define IMAGE_TYPE (FLOAT_SH(1)|COLORSPACE_SH(PT_Lab)|CHANNELS_SH(3)|BYTES_SH(sizeof(SAMPLE) & 0x07))

namespace fs = boost::filesystem;

namespace PhotoFinish {

  //! Abstract base class for image files
  class ImageFile {
  protected:
    const fs::path _filepath;
    bool _is_open;

    virtual void mark_sGrey(cmsUInt32Number intent) const = 0;
    virtual void mark_sRGB(cmsUInt32Number intent) const = 0;
    virtual void embed_icc(std::string name, unsigned char *data, unsigned int len) const = 0;
    cmsHPROFILE get_and_embed_profile(Destination::ptr dest, cmsUInt32Number cmsType, cmsUInt32Number intent);

  public:
    //! Shared pointer for an ImageFile
    typedef std::shared_ptr<ImageFile> ptr;

    //! Constructor
    /*!
      \param filepath The path of the image file
    */
    ImageFile(const fs::path filepath);

    //! Named constructor
    /*! Use the extension of the file path to decide what class to use
      \param filepath File path
    */
    static ImageFile::ptr create(const fs::path filepath) throw(UnknownFileType);

    //! Named constructor
    /*! Use the format to decide what class to use
      \param filepath File path
      \param format File format
    */
    static ImageFile::ptr create(fs::path filepath, const std::string format) throw(UnknownFileType);

    //! Create either an sRGB or greyscale profile depending on image type
    static cmsHPROFILE default_profile(cmsUInt32Number cmsType);

    //! Add variables to one of the configuration objects based on destination format
    static void add_variables(Destination::ptr dest, multihash& vars);

    //! File path of this image file
    inline virtual const fs::path filepath(void) const { return _filepath; }

    //! Read the file into an image
    /*!
      \return A new Image object
    */
    virtual Image::ptr read(void);

    //! Read the file into an image
    /*!
      \param dest A Destination object where some information from the file will be placed
      \return A new Image object
    */
    virtual Image::ptr read(Destination::ptr dest) = 0;

    //! Write an image to the file
    /*!
      \param img The Image object to write
      \param dest A Destination object, used for the JPEG/PNG/etc parameters
      \param can_free Can each row of the image be freed after it is written?
    */
    virtual void write(Image::ptr img, Destination::ptr dest, bool can_free = false) = 0;
  };

#ifdef HAZ_PNG
  //! PNG file reader and writer
  class PNGfile : public ImageFile {
  private:
    png_structp _png;
    png_infop _info;

    void mark_sGrey(cmsUInt32Number intent) const;
    void mark_sRGB(cmsUInt32Number intent) const;
    void embed_icc(std::string name, unsigned char *data, unsigned int len) const;

  public:
    PNGfile(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  };
#endif

#ifdef HAZ_JPEG
  //! JPEG file reader and writer
  class JPEGfile : public ImageFile {
  private:
    jpeg_decompress_struct *_dinfo;
    jpeg_compress_struct *_cinfo;

    void mark_sGrey(cmsUInt32Number intent) const;
    void mark_sRGB(cmsUInt32Number intent) const;
    void embed_icc(std::string name, unsigned char *data, unsigned int len) const;

  public:
    JPEGfile(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
    //! Special version of write() that takes an open ostream object
    void write(std::ostream& ofs, Image::ptr img, Destination::ptr dest, bool can_free = false);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  };
#endif

#ifdef HAZ_TIFF
  //! TIFF file reader and writer
  class TIFFfile : public ImageFile {
  private:
    TIFF *_tiff;

    void mark_sGrey(cmsUInt32Number intent) const;
    void mark_sRGB(cmsUInt32Number intent) const;
    void embed_icc(std::string name, unsigned char *data, unsigned int len) const;

  public:
    TIFFfile(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  };
#endif

#ifdef HAZ_JP2
  //! JPEG 2000 file reader and writer
  class JP2file : public ImageFile {
  private:
    opj_image_t *_jp2_image;

    void mark_sGrey(cmsUInt32Number intent) const;
    void mark_sRGB(cmsUInt32Number intent) const;
    void embed_icc(std::string name, unsigned char *data, unsigned int len) const;

  public:
    JP2file(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  };
#endif

}

#endif // __IMAGEFILE_HH__
