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
#include <exiv2/exiv2.hpp>
#include "JPEG.hh"

namespace PhotoFinish {

  inline short unsigned int read_be16(const unsigned char* data) {
    return ((short unsigned int)data[0] << 8) | data[1];
  }

  inline unsigned int read_be32(const unsigned char* data) {
    return ((unsigned int)data[0] << 24) | ((unsigned int)data[1] << 16) | ((unsigned int)data[2] << 8) | data[3];
  }

  void jpeg_read_metadata(jpeg_decompress_struct* dinfo, Image::ptr img) {
    unsigned int ps3_size = 0;
    std::list<jpeg_marker_struct*> ps3_markers;

    for (jpeg_marker_struct *marker = dinfo->marker_list; marker != NULL; marker = marker->next) {
      if ((marker->marker == JPEG_APP0 + 1)
	  && (marker->data_length > 6)
	  && ((memcmp(marker->data, "Exif\0\0", 6) == 0) || (memcmp(marker->data, "Exif\0\377", 6) == 0))) {
	std::cerr << "\tReading EXIF tags..." << std::endl;
	Exiv2::ExifParser::decode(img->EXIFtags(), marker->data + 6, marker->data_length - 8);
	continue;
      }
      if ((marker->marker == JPEG_APP0 + 1)
	  && (marker->data_length > 29)
	  && (memcmp(marker->data, "http://ns.adobe.com/xap/1.0/\0", 29) == 0)) {
	std::cerr << "\tReading XMP tags..." << std::endl;
	Exiv2::XmpParser::decode(img->XMPtags(), std::string((char*)marker->data + 29, marker->data_length - 29));
	continue;
      }
      if ((marker->marker == JPEG_APP0 + 13)
	  && (marker->data_length > 14)
	  && (memcmp(marker->data, "Photoshop 3.0\0", 14) == 0)) {
	ps3_size += marker->data_length - 14;
	ps3_markers.push_back(marker);
      }
    }

    if (ps3_markers.size() > 0) {
      std::cerr << "\tFound " << ps3_markers.size() << " Photoshop 3.0 markers (" << ps3_size << " bytes)." << std::endl;
      unsigned char *ps3_data = (unsigned char*)malloc(ps3_size);
      unsigned char *pos = ps3_data;
      for (auto marker : ps3_markers) {
	memcpy(pos, marker->data + 14, marker->data_length - 14);
	pos += marker->data_length - 14;
      }

      unsigned char *iptc_data = NULL;
      unsigned int iptc_size = 0;
      unsigned int position = 0;
      while (position + 12 <= ps3_size) {
	position += 4;
	short unsigned int type = read_be16(ps3_data + position);
	position += 2;

	// Pascal string is padded to have an even size (including size byte)
	unsigned char string_size = ps3_data[position] + 1;
	string_size += (string_size & 1);
	position += string_size;
	if (position + 4 > ps3_size) {
	  std::cerr << "\t\tWarning: Invalid or extended Photoshop IRB" << std::endl;
	  break;
	}

	unsigned int data_size = read_be32(ps3_data + position);
	position += 4;
	if (position + data_size > ps3_size) {
	  std::cerr << "\t\tWarning: Invalid Photoshop IRB data size "
		    << data_size << " or extended Photoshop IRB" << std::endl;
	  break;
	}

	if ((data_size & 1) && (position + data_size == ps3_size)) {
	  std::cerr << "\t\tWarning: Photoshop IRB data is not padded to even size" << std::endl;
	}

	if (type == 0x0404) {
	  std::cerr << "\t\tFound an IPTC IRB (" << data_size << " bytes)." << std::endl;
	  iptc_data = (unsigned char*)realloc(iptc_data, iptc_size + data_size);
	  memcpy(iptc_data + iptc_size, ps3_data + position, data_size);
	  iptc_size += data_size;
	}

	// Data size is also padded to be even
	position += data_size + (data_size & 1);
      }

      if (iptc_size > 0) {
	std::cerr << "\tReading IPTC tags..." << std::endl;
	Exiv2::IptcParser::decode(img->IPTCtags(), iptc_data, iptc_size);
	free(iptc_data);
      }
      free(ps3_data);
    }

  }

} // namespace PhotoFinish
