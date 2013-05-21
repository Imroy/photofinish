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
#include "WebP_ostream.hh"

namespace PhotoFinish {

  webp_stream_writer::webp_stream_writer(std::ostream* s, unsigned int w, unsigned int h) :
    stream(s), chunk_size(4), next_chunk(12), header_at_start(false),
    need_vp8x(false), width(w - 1), height(h - 1),
    icc_data(NULL), exif_data(NULL), xmp_data(NULL),
    icc_size(0), exif_size(0), xmp_size(0) {
    memcpy(fourcc, "   ", 4);
  }

  webp_stream_writer::~webp_stream_writer() {
    if (icc_data)
      free(icc_data);
    if (exif_data)
      free(exif_data);
    if (xmp_data)
      free(xmp_data);
  }

    //! Add an LCMS2 profile to be written
  void webp_stream_writer::add_icc(cmsHPROFILE profile) {
    cmsSaveProfileToMem(profile, NULL, &icc_size);
    if (icc_size > 0) {
      icc_data = (unsigned char*)malloc(icc_size);
      cmsSaveProfileToMem(profile, icc_data, &icc_size);
    }
    need_vp8x = true;
  }

    //! Add a set of EXIF tags to be written
  void webp_stream_writer::add_exif(const Exiv2::ExifData& exif) {
    Exiv2::Blob blob;
    Exiv2::ExifParser::encode(blob, Exiv2::littleEndian, exif);
    exif_size = blob.size();
    unsigned char *data = exif_data = (unsigned char*)malloc(exif_size);
    for (auto i : blob)
      *data++ = i;
    need_vp8x = true;
  }

    //! Add a set of XMP tags to be written
  void webp_stream_writer::add_xmp(const Exiv2::XmpData& xmp) {
    std::string s;
    if (Exiv2::XmpParser::encode(s, xmp) == 0) {
      xmp_size = s.length() + 1;
      xmp_data = (unsigned char*)malloc(xmp_size);
      memcpy(xmp_data, s.c_str(), xmp_size);
      need_vp8x = true;
    }
  }

    //! Write a RIFF chunk
  void webp_stream_writer::write_chunk(const char *fourcc, const unsigned char* data, unsigned int length) {
    stream->write(fourcc, 4);
    unsigned char l[4];
    copy_le_to(l, length, 4);
    stream->write((char*)l, 4);
    stream->write((char*)data, length);
    if (length & 0x01)
      stream->put(0);
  }

  void webp_stream_writer::before_chunk(void) {
    if (need_vp8x && ((memcmp(fourcc, "VP8 ", 4) == 0) || (memcmp(fourcc, "VP8L", 4) == 0))) {
      std::cerr << "\tInserting VP8X chunk..." << std::endl;
      unsigned char vp8x[10];
      memset(vp8x, 0, 4);
      modify_vp8x(vp8x);
      copy_le_to(vp8x + 4, width, 3);
      copy_le_to(vp8x + 7, height, 3);
      write_chunk("VP8X", vp8x, 10);

      if (icc_size) {
	std::cerr << "\tInserting ICCP chunk (" << icc_size << " bytes)..." << std::endl;
	write_chunk("ICCP", icc_data, icc_size);
      }
      need_vp8x = false;
    }
  }

  void webp_stream_writer::modify_chunk(unsigned char* data) {
    if (need_vp8x && (memcmp(data, "VP8X", 4) == 0)) {
      std::cerr << "\tModifying VP8X chunk..." << std::endl;
      modify_vp8x(data + 8);
      need_vp8x = false;
    }
  }

  void webp_stream_writer::after_chunk(void) {
    if (memcmp(fourcc, "VP8X", 4) == 0) {
      std::cerr << "\tInserting ICCP chunk (" << icc_size << " bytes)..." << std::endl;
      write_chunk("ICCP", icc_data, icc_size);
    }
    if ((memcmp(fourcc, "VP8 ", 4) == 0) || (memcmp(fourcc, "VP8L", 4) == 0)) {
      if (exif_size) {
	std::cerr << "\tInserting EXIF chunk (" << exif_size << " bytes)..." << std::endl;
	write_chunk("EXIF", exif_data, exif_size);
      }
      if (xmp_size) {
	std::cerr << "\tInserting XMP chunk (" << xmp_size << " bytes)..." << std::endl;
	write_chunk("XMP ", xmp_data, xmp_size);
      }
    }
  }

  void webp_stream_writer::modify_vp8x(unsigned char* data) {
    if (icc_size)
      *data |= 1 << 5;
    if (exif_size)
      *data |= 1 << 3;
    if (xmp_size)
      *data |= 1 << 2;
  }

#define min(a, b) ((a) < (b) ? (a) : (b))

  //! Write a block of data from the encoder
  int webp_stream_writer::write(unsigned char* data, size_t data_size) {
    // First write the end of any chunk in the buffer
    if (!header_at_start) {
      unsigned int size = min(next_chunk, data_size);
      std::cerr << "\tWriting " << size << " bytes..." << std::endl;
      stream->write((char*)data, size);
      data += size;
      data_size -= size;
      next_chunk -= size;
    }
    header_at_start = false;

    // Work through every chunk in the buffer
    while ((next_chunk < data_size) && (data_size >= 8)) {
      memcpy(fourcc, data, 4);
      chunk_size = read_le32(data + 4);
      next_chunk = 8 + chunk_size + (chunk_size & 0x1);

      before_chunk();

      std::cerr << "\tFound chunk: \"" << std::string((char*)fourcc, 4) << "\", " << chunk_size << " bytes." << std::endl;

      modify_chunk(data);

      // Write a whole chunk if we have it
      if (next_chunk < data_size) {
	unsigned int total_size = 8 + chunk_size + (chunk_size & 0x01);
	std::cerr << "\tWriting " << total_size << " bytes." << std::endl;
	stream->write((char*)data, total_size);
	data += total_size;
	data_size -= total_size;
	next_chunk = 0;
	after_chunk();
      }
    }
    // Write the rest of the data
    if (data_size > 0) {
      std::cerr << "\tWriting " << data_size << " bytes..." << std::endl;
      stream->write((char*)data, data_size);
      next_chunk -= data_size;
    }

    // The end of the buffer coincides with the end of a chunk
    if (next_chunk == 0) {
      after_chunk();
      header_at_start = true;
    }

    return 1;
  }

}; // namespace PhotoFinish
