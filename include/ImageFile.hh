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
#include <lcms2.h>
#include <memory>
#include <boost/filesystem.hpp>
#include "Image.hh"
#include "Destination.hh"
#include "Tags.hh"
#include "Exception.hh"
#include "sample.h"

#define IMAGE_TYPE (FLOAT_SH(1)|COLORSPACE_SH(PT_Lab)|CHANNELS_SH(3)|BYTES_SH(sizeof(SAMPLE) & 0x07))

namespace fs = boost::filesystem;

namespace PhotoFinish {

  //! Abstract base class for image files
  class Base_ImageFile {
  protected:
    const fs::path _filepath;

  public:
    //! Empty constructor
    Base_ImageFile() :
      _filepath()
    {}

    //! Constructor
    /*!
      \param filepath The path of the image file
    */
    Base_ImageFile(const fs::path filepath) :
      _filepath(filepath)
    {}

    //! Read the file into an image
    /*!
      \return A new Image object
    */
    virtual Image::ptr read(void) const = 0;

    //! Write an image to the file
    /*!
      \param img The Image object to write
      \param dest A Destination object, used for the JPEG/PNG/etc parameters
      \param tags A Tags object, used for writing EXIF/IPTC/XMP metadata to the new file
    */
    virtual void write(Image::ptr img, const Destination &dest, const Tags &tags) const = 0;
  };

  //! PNG file reader and writer
  class PNGFile : public Base_ImageFile {
  private:

  public:
    PNGFile(const fs::path filepath);

    Image::ptr read(void) const;
    void write(Image::ptr img, const Destination &dest, const Tags &tags) const;
  };

  //! JPEG file writer
  class JPEGFile : public Base_ImageFile {
  private:

  public:
    JPEGFile(const fs::path filepath);

    Image::ptr read(void) const;
    void write(Image::ptr img, const Destination &dest, const Tags &tags) const;
  };

  //! Factory/wrapper class for creating image file objects
  class ImageFile : public Base_ImageFile {
  private:
    typedef std::shared_ptr<Base_ImageFile> ptr;

    ptr _imagefile;

  public:
    //! Constructor
    /*!
      \param filepath File path, extension is used to decide what class to use
    */
    ImageFile(const fs::path filepath) throw(UnknownFileType);

    //! Constructor
    /*!
      \param filepath File path
      \param format Is used to decide what class to use
    */
    ImageFile(fs::path filepath, const std::string format) throw(UnknownFileType);

    Image::ptr read(void) const;
    void write(Image::ptr img, const Destination &dest, const Tags &tags) const;
  };

}

#endif // __IMAGEFILE_HH__
