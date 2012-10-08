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
#include <lcms2.h>
#include "ImageFile.hh"
#include "Image.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {
  SOLwriter::SOLwriter(std::ostream& os) :
    ImageWriter(os)
  {}

  unsigned char solheader[12] = { 0x53, 0x4f, 0x4c, 0x3a, 0x00, 0x00, 0x00, 0x00,
			       0x00, 0x00, 0x00, 0x00 };

  void write_be(void *ptr, size_t size, std::ostream &stream) {
    unsigned char *in = (unsigned char*)ptr + size - 1;
    size_t n;
    for (n = 0; n < size; n++, in--)
      stream.put(*in);
  }

  void SOLwriter::receive_image_header(ImageHeader::ptr header) {
    ImageSink::receive_image_header(header);
    
    _os.write((char*)solheader, 12);
    {
      unsigned int width = header->width(), height = header->height();
      write_be(&width, 4, _os);
      write_be(&height, 4, _os);
    }

    if (T_COLORSPACE(_sink_header->cmsType()) != PT_RGB)
      throw CMSError("Not RGB colour space");

    if (T_CHANNELS(_sink_header->cmsType()) != 3)
      throw CMSError("Not 3 channels");

    if (T_BYTES(_sink_header->cmsType()) != 1)
      throw CMSError("Not 1 byte per pixel");
  }

  void SOLwriter::_work_on_row(ImageRow::ptr row) {
    unsigned char *inp = (unsigned char*)row->data();
    for (unsigned int x = 0; x < _sink_header->width(); x++) {
      unsigned char r = *inp++;
      unsigned char g = *inp++;
      unsigned char b = *inp++;
      _os.put(b | ((g & 0x07) << 5));
      _os.put((g >> 3) | (r << 3));
    }
  }

  void SOLwriter::receive_image_end(void) {
    ImageSink::receive_image_end();
  }

}
