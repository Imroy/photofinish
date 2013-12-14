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
#ifdef HAZ_WEBP
#include <webp/encode.h>
#include <webp/decode.h>
#endif

#include "CMS.hh"
#include "Image.hh"
#include "Destination.hh"
#include "Exception.hh"
#include "sample.h"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  //! Class for holding filename and the image format
  class ImageFilepath {
  private:
    fs::path _filepath;
    std::string _format;

  public:
    //! Constructor
    /*!
      \param filepath The path of the image file
      \param format Format of the image file
    */
    ImageFilepath(const fs::path filepath, const std::string format);

    //! Constructor
    /*!
      Guess the format from the file extension.
      \param filepath The path of the image file
    */
    ImageFilepath(const fs::path filepath) throw(UnknownFileType);

    fs::path fixed_filepath(void) const throw(UnknownFileType);

    inline void fix_filepath(void) throw(UnknownFileType) { _filepath = fixed_filepath(); }

    //! File path of this image file
    inline virtual const fs::path filepath(void) const { return _filepath; }

    //! Format of this image file
    inline virtual std::string format(void) const { return _format; }

    inline friend std::ostream& operator << (std::ostream& out, const ImageFilepath& fp) {
      out << fp._filepath << "(" << fp._format << ")";
      return out;
    }

  }; // class ImageFilepath

  inline bool exists(const ImageFilepath& fp) { return exists(fp.filepath()); }
  inline std::time_t last_write_time(const ImageFilepath& fp) { return last_write_time(fp.filepath()); }

  //! Abstract base class for reading image files
  class ImageReader {
  protected:
    const fs::path _filepath;
    bool _is_open;

    //! Private constructor
    ImageReader(const fs::path fp);

    //! Extract tags from file
    void extract_tags(Image::ptr img);

  public:
    //! Shared pointer for an ImageReader
    typedef std::shared_ptr<ImageReader> ptr;

    //! Named constructor
    /*! Use the extension of the file path to decide what class to use
      \param filepath File path
    */
    static ImageReader::ptr open(const ImageFilepath& ifp) throw(UnknownFileType);

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

  }; // class ImageReader



  //! Abstract base class for writing image files
  class ImageWriter {
  protected:
    const fs::path _filepath;
    bool _is_open;

    //! Private constructor
    ImageWriter(const fs::path fp);

    void embed_tags(Image::ptr img) const;

  public:
    //! Shared pointer for an ImageWriter
    typedef std::shared_ptr<ImageWriter> ptr;

    //! Named constructor
    /*! Use the extension of the file path to decide what class to use
      \param filepath File path
    */
    static ImageWriter::ptr open(const ImageFilepath& ifp) throw(UnknownFileType);

    //! Add variables to one of the configuration objects based on destination format
    static void add_variables(Destination::ptr dest, multihash& vars);

    //! Modify an LCMS2 pixel format into a "type" that the file format can write
    virtual CMS::Format preferred_format(CMS::Format format) = 0;

    //! Write an image to the file
    /*!
      \param img The Image object to write
      \param dest A Destination object, used for the JPEG/PNG/etc parameters
      \param can_free Can each row of the image be freed after it is written?
    */
    virtual void write(Image::ptr img, Destination::ptr dest, bool can_free = false) = 0;

  }; // class ImageWriter



#ifdef HAZ_PNG
  //! PNG file reader
  class PNGreader : public ImageReader {
  private:
    png_structp _png;
    png_infop _info;

  public:
    PNGreader(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
  }; // class PNGreader


  //! PNG file writer
  class PNGwriter : public ImageWriter {
  private:
    png_structp _png;
    png_infop _info;

  public:
    PNGwriter(const fs::path filepath);

    CMS::Format preferred_format(CMS::Format format);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  }; // class PNGwriter
#endif // HAZ_PNG

#ifdef HAZ_JPEG
  //! JPEG file reader
  class JPEGreader : public ImageReader {
  private:

  public:
    JPEGreader(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
  }; // class JPEGreader


  //! JPEG file writer
  class JPEGwriter : public ImageWriter {
  private:

  public:
    JPEGwriter(const fs::path filepath);

    CMS::Format preferred_format(CMS::Format format);
    //! Special version of write() that takes an open ostream object
    void write(std::ostream& ofs, Image::ptr img, Destination::ptr dest, bool can_free = false);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  }; // class JPEGwriter
#endif // HAZ_JPEG

#ifdef HAZ_TIFF
  //! TIFF file reader
  class TIFFreader : public ImageReader {
  private:

  public:
    TIFFreader(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
  }; // class TIFFreader


  //! TIFF file writer
  class TIFFwriter : public ImageWriter {
  private:

  public:
    TIFFwriter(const fs::path filepath);

    CMS::Format preferred_format(CMS::Format format);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  }; // class TIFFwriter
#endif // HAZ_TIFF

#ifdef HAZ_JP2
  //! JPEG 2000 file reader
  class JP2reader : public ImageReader {
  private:

  public:
    JP2reader(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
  }; // class JP2reader


  //! JPEG 2000 file writer
  class JP2writer : public ImageWriter {
  private:

  public:
    JP2writer(const fs::path filepath);

    CMS::Format preferred_format(CMS::Format format);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  }; // class JP2writer
#endif // HAZ_JP2

#ifdef HAZ_WEBP
  //! WebP file reader
  class WebPreader : public ImageReader {
  private:

  public:
    WebPreader(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
  }; // class WebPreader


  //! WebP file writer
  class WebPwriter : public ImageWriter {
  private:

  public:
    WebPwriter(const fs::path filepath);

    CMS::Format preferred_format(CMS::Format format);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  }; // class WebPwriter
#endif // HAZ_WEBP

#ifdef HAZ_JXR
  //! JPEG XR reader
  class JXRreader : public ImageReader {
  private:

  public:
    JXRreader(const fs::path filepath);

    Image::ptr read(Destination::ptr dest);
  }; // class JXRreader


  //! JPEG XR writer
  class JXRwriter : public ImageWriter {
  private:

  public:
    JXRwriter(const fs::path filepath);

    CMS::Format preferred_format(CMS::Format format);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  }; // class JXRwriter

#endif // HAZ_JXR

  //! Write the boot logo files for use on Motorola Atrix 4G and possibly other phones
  /*!
    I haven't been able to find any documentation about this format.
    It starts with the ASCII string "SOL:" followed by eight null bytes.
    Then comes the width and height as big-endian 32-bit values.
    The image data is as uncompressed 5-6-5 bit pixels i.e 16 bits per pixel.
    No footer.
   */
  class SOLwriter : public ImageWriter {
  private:

  public:
    SOLwriter(const fs::path filepath);

    CMS::Format preferred_format(CMS::Format format);
    void write(Image::ptr img, Destination::ptr dest, bool can_free = false);
  }; // class SOLwriter

}

#endif // __IMAGEFILE_HH__
