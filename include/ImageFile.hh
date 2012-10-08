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

  //! Class for holding filename and the image format
  class ImageFilepath {
  protected:
    const fs::path _filepath;
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

    //! File path of this image file
    inline virtual fs::path filepath(void) const { return _filepath; }

    //! Format of this image file
    inline virtual std::string format(void) const { return _format; }
  }; // ImageFilepath



  //! Base class for reading image files
  class ImageReader : public ImageSource, public Worker {
  protected:
    std::istream *_is;
    int _read_state;

    ImageReader(std::istream* is);

  public:
    typedef std::shared_ptr<ImageReader> ptr;

    //! Named constructor
    /*! Use the extension of the file path to decide what class to use
      \param filepath File path
    */
    static ImageReader::ptr open(const ImageFilepath ifp) throw(UnknownFileType);

    virtual const std::string format(void) const = 0;

    //! Read the image file and send header, rows, and finally the end to registered sinks
    virtual void do_work(void) = 0;
  }; // class ImageReader



  //! Base class for writing image files
  class ImageWriter : public ImageSink {
  protected:
    std::ostream *_os;
    Destination::ptr _dest;

    ImageWriter(std::ostream* os, Destination::ptr dest);

    virtual void mark_sGrey(cmsUInt32Number intent) const = 0;
    virtual void mark_sRGB(cmsUInt32Number intent) const = 0;
    virtual void embed_icc(std::string name, unsigned char *data, unsigned int len) const = 0;
    cmsHPROFILE get_and_embed_profile(Destination::ptr dest, cmsUInt32Number cmsType, cmsUInt32Number intent);

    inline virtual void _work_on_row(ImageRow::ptr row) {}

  public:
    typedef std::shared_ptr<ImageWriter> ptr;

    //! Named constructor
    /*! Use the extension of the file path to decide what class to use
      \param filepath File path
    */
    static ImageWriter::ptr open(const ImageFilepath ifp, Destination::ptr dest) throw(UnknownFileType);

    virtual const std::string format(void) const = 0;

    virtual void receive_image_header(ImageHeader::ptr header);
    virtual void receive_image_row(ImageRow::ptr row);
    virtual void receive_image_end(void);
    virtual void do_work(void) = 0;
  }; // class ImageWriter



  //! Create either an sRGB or greyscale profile depending on image type
  cmsHPROFILE default_profile(cmsUInt32Number cmsType);

  //! Add variables to one of the configuration objects based on destination format
  void add_format_variables(Destination::ptr dest, multihash& vars);



#ifdef HAZ_PNG
  //! PNG file reader
  class PNGreader : public ImageReader {
  private:
    png_structp _png;
    png_infop _info;
    unsigned char *_buffer;

    PNGreader(std::istream* is);
    friend class ImageReader;

    ~PNGreader();

    friend void png_info_cb(png_structp png, png_infop info);
    friend void png_row_cb(png_structp png, png_bytep row_data, png_uint_32 row_num, int pass);
    friend void png_end_cb(png_structp png, png_infop info);

  public:
    inline const std::string format(void) const { return "png"; }

    void do_work(void);
  };

  //! PNG file writer
  class PNGwriter : public ImageWriter {
  private:
    png_structp _png;
    png_infop _info;
    unsigned int _next_y;

    void mark_sGrey(cmsUInt32Number intent) const;
    void mark_sRGB(cmsUInt32Number intent) const;
    void embed_icc(std::string name, unsigned char *data, unsigned int len) const;

    PNGwriter(std::ostream* os, Destination::ptr dest);
    friend class ImageWriter;

  public:
    inline const std::string format(void) const { return "png"; }

    virtual void receive_image_header(ImageHeader::ptr header);

    virtual void do_work(void);

    virtual void receive_image_end(void);
  };
#endif

#ifdef HAZ_JPEG
  //! JPEG file reader
  class JPEGreader : public ImageReader {
  private:
    jpeg_decompress_struct *_dinfo;

    JPEGreader(std::istream* is);
    friend class ImageReader;

  public:
    inline const std::string format(void) const { return "jpeg"; }

    void do_work(void);
  };

  //! JPEG file writer
  class JPEGwriter : public ImageWriter {
  private:
    jpeg_compress_struct *_cinfo;

    void mark_sGrey(cmsUInt32Number intent) const;
    void mark_sRGB(cmsUInt32Number intent) const;
    void embed_icc(std::string name, unsigned char *data, unsigned int len) const;

    JPEGwriter(std::ostream* os, Destination::ptr dest);
    friend class ImageWriter;

  public:
    inline const std::string format(void) const { return "jpeg"; }

    virtual void receive_image_header(ImageHeader::ptr header);

    virtual void do_work(void);

    virtual void receive_image_end(void);
  };
#endif

#ifdef HAZ_TIFF
  //! TIFF file reader
  class TIFFreader : public ImageReader {
  private:
    TIFF *_tiff;
    unsigned int _next_y;

    TIFFreader(std::istream* is);
    friend class ImageReader;

  public:
    inline const std::string format(void) const { return "tiff"; }

    void do_work(void);
  };

  //! TIFF file writer
  class TIFFwriter : public ImageWriter {
  private:
    TIFF *_tiff;
    unsigned int _next_y;

    void mark_sGrey(cmsUInt32Number intent) const;
    void mark_sRGB(cmsUInt32Number intent) const;
    void embed_icc(std::string name, unsigned char *data, unsigned int len) const;

    TIFFwriter(std::ostream* os, Destination::ptr dest);
    friend class ImageWriter;

  public:
    inline const std::string format(void) const { return "tiff"; }

    virtual void receive_image_header(ImageHeader::ptr header);

    virtual void do_work(void);

    virtual void receive_image_end(void);
  };
#endif

#ifdef HAZ_JP2
  //! JPEG 2000 file reader
  class JP2reader : public ImageReader {
  private:
    opj_dinfo_t *_dinfo;
    unsigned char *_src;
    opj_cio_t *_cio;
    opj_image_t *_jp2_image;
    unsigned int _width, _height, _next_y;

    JP2reader(std::istream* is);
    friend class ImageReader;

  public:
    inline const std::string format(void) const { return "jp2"; }

    void do_work(void);
  };

  //! JPEG 2000 file writer
  class JP2writer : public ImageWriter {
  private:
    opj_cparameters_t _parameters;
    opj_image_t *_jp2_image;
    unsigned int _width, _height, _next_y;

    void mark_sGrey(cmsUInt32Number intent) const;
    void mark_sRGB(cmsUInt32Number intent) const;
    void embed_icc(std::string name, unsigned char *data, unsigned int len) const;

    JP2writer(std::ostream* os, Destination::ptr dest);
    friend class ImageWriter;

  public:
    inline const std::string format(void) const { return "jp2"; }

    virtual void receive_image_header(ImageHeader::ptr header);

    virtual void do_work(void);

    virtual void receive_image_end(void);
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
  class SOLwriter : public ImageWriter {
  private:
    void mark_sGrey(cmsUInt32Number intent) const {}
    void mark_sRGB(cmsUInt32Number intent) const {}
    void embed_icc(std::string name, unsigned char *data, unsigned int len) const {}

    SOLwriter(std::ostream* os, Destination::ptr dest);
    friend class ImageWriter;

  public:
    inline const std::string format(void) const { return "sol"; }

    virtual void receive_image_header(ImageHeader::ptr header);

    virtual void do_work(void);

    virtual void receive_image_end(void);
  };

}

#endif // __IMAGEFILE_HH__
