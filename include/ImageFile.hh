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

namespace fs = boost::filesystem;

namespace PhotoFinish {

  //! Abstract base class for image files
  class ImageFile {
  protected:
    const fs::path _filepath;
    bool _is_open;

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

    //! Modify an LCMS2 pixel format into a "type" that the file format can write
    virtual cmsUInt32Number preferred_type(cmsUInt32Number type = 0) = 0;

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

  public:
    PNGfile(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
    cmsUInt32Number preferred_type(cmsUInt32Number type);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  };
#endif

#ifdef HAZ_JPEG
  //! JPEG file reader and writer
  class JPEGfile : public ImageFile {
  private:

  public:
    JPEGfile(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
    cmsUInt32Number preferred_type(cmsUInt32Number type);
    //! Special version of write() that takes an open ostream object
    void write(std::ostream& ofs, Image::ptr img, Destination::ptr dest, bool can_free = false);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  };
#endif

#ifdef HAZ_TIFF
  //! TIFF file reader and writer
  class TIFFfile : public ImageFile {
  private:

  public:
    TIFFfile(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
    cmsUInt32Number preferred_type(cmsUInt32Number type);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  };
#endif

#ifdef HAZ_JP2
  //! JPEG 2000 file reader and writer
  class JP2file : public ImageFile {
  private:

  public:
    JP2file(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
    cmsUInt32Number preferred_type(cmsUInt32Number type);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  };
#endif

  //! Write the boot logo files for use on Motorola Atrix 4G and possibly other phones
  /*!
    I haven't been able to find any documentation about this format.
    It starts with the ASCII string "SOL:" followed by eight null bytes.
    Then comes the width and height as big-endian 32-bit values.
    The image data is as uncompressed 5-6-5 bit pixels i.e 16 bits per pixel.
    No footer.
   */
  class SOLfile : public ImageFile {
  private:

  public:
    SOLfile(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
    cmsUInt32Number preferred_type(cmsUInt32Number type);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  };

}

#endif // __IMAGEFILE_HH__
