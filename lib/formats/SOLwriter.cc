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
#include "ImageFile.hh"
#include "Image.hh"
#include "Ditherer.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {
  SOLwriter::SOLwriter(const fs::path filepath) :
    ImageWriter(filepath)
  {}

  CMS::Format SOLwriter::preferred_format(CMS::Format format) {
    format.set_colour_model(CMS::ColourModel::RGB);

    format.set_packed();

    format.set_extra_channels(0);
    format.set_premult_alpha();

    format.set_16bit();

    return format;
  }

  unsigned char header[12] = { 0x53, 0x4f, 0x4c, 0x3a, 0x00, 0x00, 0x00, 0x00,
			       0x00, 0x00, 0x00, 0x00 };

  void write_be(void *ptr, size_t size, std::ostream &stream) {
    unsigned char *in = (unsigned char*)ptr + size - 1;
    size_t n;
    for (n = 0; n < size; n++, in--)
      stream.put(*in);
  }

  void SOLwriter::write(Image::ptr img, Destination::ptr dest, bool can_free) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ofstream ofs;
    ofs.open(_filepath, std::ios_base::out);

    ofs.write((char*)header, 12);
    {
      unsigned int width = img->width(), height = img->height();
      write_be(&width, 4, ofs);
      write_be(&height, 4, ofs);
    }

    CMS::Format format = img->format();
    if (format.colour_model() != CMS::ColourModel::RGB)
      throw cmsTypeError("Not RGB", format);
    if (format.bytes_per_channel() != 2)
      throw cmsTypeError("Not 16-bit", format);

    Ditherer ditherer(img->width(), 3, { 31, 63, 31 });
    unsigned char *temprow = new unsigned char[img->width() * 3];

    for (unsigned int y = 0; y < img->height(); y++) {
      ditherer.dither(img->row(y)->data<short unsigned int>(), temprow, y == img->height() - 1);
      if (can_free)
	img->free_row(y);

      unsigned char *inp = temprow;
      for (unsigned int x = 0; x < img->width(); x++) {
	unsigned char r = *inp++;
	unsigned char g = *inp++;
	unsigned char b = *inp++;
	ofs.put(b | ((g & 0x07) << 5));
	ofs.put((g >> 3) | (r << 3));
      }
      std::cerr << "\r\tWrote " << y + 1 << " of " << img->height() << " rows";
    }
    std::cerr << "\r\tWrote " << img->height() << " of " << img->height() << " rows." << std::endl;

    delete [] temprow;

    ofs.close();
    _is_open = false;

    std::cerr << "Done." << std::endl;
  }
}
