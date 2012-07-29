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
#include <boost/filesystem/fstream.hpp>
#include "Image.hh"
#include "Destination.hh"
#include "Tags.hh"
#include "Exception.hh"
#include "sample.h"

#define IMAGE_TYPE (FLOAT_SH(1)|COLORSPACE_SH(PT_Lab)|CHANNELS_SH(3)|BYTES_SH(sizeof(SAMPLE) & 0x07))

namespace fs = boost::filesystem;

namespace PhotoFinish {

  //! Abstract base class for image files
  class ImageFile {
  protected:
    const fs::path _filepath;

  public:
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

    //! Accessor
    inline virtual const fs::path filepath(void) const { return _filepath; }

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
  class PNGFile : public ImageFile {
  private:

  public:
    PNGFile(const fs::path filepath);

    Image::ptr read(void) const;
    void write(Image::ptr img, const Destination &dest, const Tags &tags) const;
  };

  //! JPEG file writer
  class JPEGFile : public ImageFile {
  private:

  public:
    JPEGFile(const fs::path filepath);

    Image::ptr read(void) const;
    //! Special version of write() that takes an open ostream object
    void write(std::ostream& ofs, Image::ptr img, const Destination &dest) const;
    void write(Image::ptr img, const Destination &dest, const Tags &tags) const;
  };

  //! TIFF file reader and writer
  class TIFFfile : public ImageFile {
  private:

  public:
    TIFFfile(const fs::path filepath);

    Image::ptr read(void) const;
    void write(Image::ptr img, const Destination &dest, const Tags &tags) const;
  };

}

#endif // __IMAGEFILE_HH__
