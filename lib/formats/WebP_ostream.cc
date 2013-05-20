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
    stream(s), chunk_size(0), next_chunk(12),
    need_vp8x(false), width(w - 1), height(h - 1),
    icc_data(NULL), exif_data(NULL), xmp_data(NULL),
    icc_size(0), exif_size(0), xmp_size(0) {
    memcpy(chunk, "   ", 4);
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
    Exiv2::XmpParser::encode(s, xmp);
    xmp_size = s.length() + 1;
    xmp_data = (unsigned char*)malloc(xmp_size);
    memcpy(xmp_data, s.c_str(), xmp_size);
    need_vp8x = true;
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

  void webp_stream_writer::modify_vp8x(unsigned char* data) {
    if (icc_size)
      *data |= 1 << 5;
    if (exif_size)
      *data |= 1 << 3;
    if (xmp_size)
      *data |= 1 << 2;
  }

    //! Write a block of data from the encoder
  int webp_stream_writer::write(unsigned char* data, size_t data_size) {
    while (next_chunk < data_size) {
      unsigned int size = read_le32(data + next_chunk + 4);
      std::cerr << "\tFound chunk: \"" << std::string((char*)data + next_chunk, 4) << "\", " << size << " bytes." << std::endl;

      if (need_vp8x && (memcmp(data + next_chunk, "VP8X", 4) == 0)) {
	std::cerr << "\tModifying VP8X chunk..." << std::endl;
	modify_vp8x(data + next_chunk + 8);
	need_vp8x = false;
      }

      if (need_vp8x && (memcmp(data + next_chunk, "VP8 ", 4) == 0)) {
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

      memcpy(chunk, data + next_chunk, 4);
      chunk_size = read_le32(data + next_chunk + 4);
      next_chunk += size + (size & 0x01) + 8;
    }
    if (data_size) {
      std::cerr << "\tWriting " << data_size << " bytes..." << std::endl;
      stream->write((char*)data, data_size);
      next_chunk -= data_size;
      //      std::cerr << "\r\tWritten " << stream->tellp() << " bytes";
    }

    // Write stuff after a chunk
    if (next_chunk == 0) {
      if (memcmp(chunk, "VP8X", 4) == 0) {
	std::cerr << "\tInserting ICCP chunk (" << icc_size << " bytes)..." << std::endl;
	write_chunk("ICCP", icc_data, icc_size);
      }
      if (memcmp(chunk, "VP8 ", 4) == 0) {
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

    return 1;
  }

}; // namespace PhotoFinish
