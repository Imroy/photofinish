/*
	Copyright 2013 Ian Tester

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
#ifndef __WEBP_OSTREAM_HH__
#define __WEBP_OSTREAM_HH__

#include <iostream>
#include <exiv2/exiv2.hpp>
#include <lcms2.h>

namespace PhotoFinish {

  //! A custom writer for libwebp that writes using a std::ostream object
  /*!
    This class is so large because libwebp does not handle metadata *at all*.
    So we have to keep track of RIFF chunks as the encoder emits them and
    insert our own, even modifying one of the chunks (VP8X).
  */
  class webp_stream_writer {
  private:
    std::ostream *stream;
    char chunk[4];
    unsigned int chunk_size;
    unsigned int next_chunk;

    // Data for the VP8X chunk
    bool need_vp8x;
    unsigned int width, height;
    unsigned char *icc_data, *exif_data, *xmp_data;	// no IPTC?
    unsigned int icc_size, exif_size, xmp_size;

  public:
    //! Constructor
    /*!
      \param s Pointer to a std::ostream derivative.
      \param w,h Width and height of the image
    */
    webp_stream_writer(std::ostream* s, unsigned int w, unsigned int h);
    ~webp_stream_writer();

    //! Add an LCMS2 profile to be written
    void add_icc(cmsHPROFILE profile);

    //! Add a set of EXIF tags to be written
    void add_exif(const Exiv2::ExifData& exif);

    //! Add a set of XMP tags to be written
    void add_xmp(const Exiv2::XmpData& xmp);

    //! Write a RIFF chunk
    void write_chunk(const char *fourcc, const unsigned char* data, unsigned int length);

    void modify_vp8x(unsigned char* data);

    //! Write a block of data from the encoder
    int write(unsigned char* data, size_t data_size);
  }; // class webp_stream_writer

  inline void copy_le_to(unsigned char* dest, unsigned int value, unsigned char length) {
    for (unsigned char i = 0; i < length; i++) {
      dest[i] = (unsigned char)(value & 0xff);
      value >>= 8;
    }
  }

}; // namespace PhotoFinish

#endif // __WEBP_OSTREAM_HH__
