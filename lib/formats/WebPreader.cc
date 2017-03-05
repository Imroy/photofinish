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
#include <boost/algorithm/string.hpp>
#include <webp/decode.h>
#include <omp.h>
#include "ImageFile.hh"
#include "Exception.hh"
#include "WebP_ostream.hh"

namespace PhotoFinish {

  WebPreader::WebPreader(const fs::path filepath) :
    ImageReader(filepath)
  {}

  Image::ptr WebPreader::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream ifs(_filepath, std::ios_base::in);
    if (ifs.fail())
      throw FileOpenError(_filepath.native());

    WebPDecBuffer decbuffer;
    WebPInitDecBuffer(&decbuffer);

    decbuffer.colorspace = MODE_RGB;
    CMS::Format format = CMS::Format::RGB8();
    CMS::Profile::ptr profile;
    Exiv2::ExifData EXIFtags;
    Exiv2::XmpData XMPtags;
    {
      char fourcc[4];
      unsigned char size_le[4];
      ifs.read(fourcc, 4);
      if (memcmp(fourcc, "RIFF", 4) != 0)
	throw FileContentError(_filepath.native(), "Not a RIFF file");

      ifs.read((char*)size_le, 4);
      unsigned int file_size = read_le32(size_le);

      ifs.read(fourcc, 4);
      if (memcmp(fourcc, "WEBP", 4) != 0)
	throw FileContentError(_filepath.native(), "Not a WebP file");

      unsigned int next_chunk = 0;
      unsigned int chunk_size;
      do {
	ifs.read(fourcc, 4);
	ifs.read((char*)size_le, 4);
	chunk_size = read_le32(size_le);
	next_chunk += 8 + chunk_size + (chunk_size & 0x01);
	std::cerr << "\tFound \"" << std::string(fourcc, 4) << "\" chunk, " << chunk_size << " bytes long." << std::endl;

	if (memcmp(fourcc, "VP8X", 4) == 0) {
	  unsigned char flags = ifs.get();
	  if (flags & (1 << 4)) {
	    decbuffer.colorspace = MODE_RGBA;
	    format.set_extra_channels(1);
	    format.set_premult_alpha();
	  }
	}
	if (memcmp(fourcc, "ICCP", 4) == 0) {
	  unsigned char *profile_data = new unsigned char[chunk_size];
	  ifs.read((char*)profile_data, chunk_size);
	  profile = std::make_shared<CMS::Profile>(profile_data, chunk_size);
	  if (profile != NULL) {
	    std::string profile_name = profile->description("en", "");
	    if (profile_name.length() > 0)
	      dest->set_profile(profile_name, profile_data, chunk_size);
	    else
	      dest->set_profile("WebP", profile_data, chunk_size);

	    std::cerr << "\tRead embedded profile \"" << dest->profile()->name().get() << "\" (" << chunk_size << " bytes)." << std::endl;
	  }
	}
	if (memcmp(fourcc, "EXIF", 4) == 0) {
	  unsigned char *data = (unsigned char*)malloc(chunk_size);
	  ifs.read((char*)data, chunk_size);
	  std::cerr << "\tReading EXIF tags..." << std::endl;
	  Exiv2::ExifParser::decode(EXIFtags, data, chunk_size);
	  free(data);
	}
	if (memcmp(fourcc, "XMP ", 4) == 0) {
	  char *data = (char*)malloc(chunk_size);
	  ifs.read(data, chunk_size);
	  std::string s(data, chunk_size);
	  std::cerr << "\tReading XMP tags..." << std::endl;
	  Exiv2::XmpParser::decode(XMPtags, s);
	  free(data);
	}

	ifs.seekg(next_chunk + 12, std::ios_base::beg);
	ifs.peek();
      } while (ifs.good() && (next_chunk < file_size));
      ifs.seekg(0, std::ios_base::beg);
    }
    WebPIDecoder* idec = WebPINewDecoder(&decbuffer);

    unsigned char buffer[1048576];
    size_t length;
    Image::ptr img;
    int width, height, y = 0, last_y, stride;
    unsigned char *rowdata;
    do {
      ifs.read((char*)buffer, 1048576);
      length = ifs.gcount();
      int status = WebPIAppend(idec, buffer, length);

      rowdata = WebPIDecGetRGB(idec, &last_y, &width, &height, &stride);
      if (rowdata != NULL) {
	if (img == NULL) {
	  std::cerr << "\t" << width << "×" << height << " RGB" << (format.extra_channels() > 0 ? "A" : "") << std::endl;
	  img = std::make_shared<Image>(width, height, format);
	}
	while (y < last_y) {
	  memcpy(img->row(y), rowdata, stride);
	  std::cerr << "\r\tRead " << (y + 1) << " of " << height << " rows";
	  y++;
	  rowdata += stride;
	}
      }

      if (status != VP8_STATUS_SUSPENDED)
	break;
    } while (!ifs.eof());
    std::cerr << "\r\tRead " << height << " of " << height << " rows." << std::endl;

    WebPIDelete(idec);

    if (profile != NULL)
      img->set_profile(profile);

    for (auto ei : EXIFtags)
      img->EXIFtags().add(ei);

    for (auto xi : XMPtags)
      img->XMPtags().add(xi);

    return img;
  }

}
