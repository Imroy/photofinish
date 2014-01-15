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
#include <errno.h>
#include <png.h>
#include <zlib.h>
#include <time.h>
#include <omp.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>
#include "ImageFile.hh"
#include "Image.hh"
#include "PNGreader_cb.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  PNGreader::PNGreader(const fs::path filepath) :
    ImageReader(filepath),
    _png(NULL), _info(NULL)
  {}

  Image::ptr PNGreader::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream ifs(_filepath, std::ios_base::in);
    if (ifs.fail())
      throw FileOpenError(_filepath.native());

    {
      unsigned char header[8];
      ifs.read((char*)header, 8);
      if (png_sig_cmp(header, 0, 8))
	throw FileContentError(_filepath.string(), "is not a PNG file");
      ifs.seekg(0, std::ios_base::beg);
    }

    _png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					     NULL, NULL, NULL);
    if (!_png)
      throw LibraryError("libpng", "Could not create PNG read structure");

    _info = png_create_info_struct(_png);
    if (!_info) {
      png_destroy_read_struct(&_png, (png_infopp)NULL, (png_infopp)NULL);
      throw LibraryError("libpng", "Could not create PNG info structure");
    }

    if (setjmp(png_jmpbuf(_png))) {
      png_destroy_read_struct(&_png, &_info, NULL);
      ifs.close();
      throw LibraryError("libpng", "Something went wrong reading the PNG");
    }

    PNGreader_cb cb(dest);

    std::cerr << "\tReading PNG image..." << std::endl;
    png_set_progressive_read_fn(_png, (void *)&cb, png_info_cb, png_row_cb, png_end_cb);
    png_byte buffer[1048576];
    size_t length;
    do {
      ifs.read((char*)buffer, 1048576);
      length = ifs.gcount();
      png_process_data(_png, _info, buffer, length);
    } while (length > 0);

    png_destroy_read_struct(&_png, &_info, NULL);
    ifs.close();
    _is_open = false;

    std::cerr << "\tExtracting tags..." << std::endl;
    extract_tags(cb._image);

    std::cerr << "Done." << std::endl;
    return cb._image;
  }

}
