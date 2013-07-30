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
#ifndef __JPEG_HH__
#define __JPEG_HH__
#include <stdio.h>
#include <jpeglib.h>
#include "CMS.hh"
#include "Image.hh"
#include "Destination.hh"

namespace PhotoFinish {

  //! Set up a "source manager" on the given JPEG decompression structure to read from an istream
  void jpeg_istream_src(j_decompress_ptr dinfo, std::istream* is);

  //! Free the data structures of the istream source manager
  void jpeg_istream_src_free(j_decompress_ptr dinfo);

  //! Setup a "destination manager" on the given JPEG compression structure to write to an ostream
  void jpeg_ostream_dest(j_compress_ptr cinfo, std::ostream* os);

  //! Free the data structures of the ostream destination manager
  void jpeg_ostream_dest_free(j_compress_ptr cinfo);

  //! Create a scan "script" for an RGB image
  void jpegfile_scan_RGB(jpeg_compress_struct* cinfo);

  //! Create a scan "script" for a greyscale image
  void jpegfile_scan_greyscale(jpeg_compress_struct* cinfo);

  //! Read EXIF, XMP, and IPTC metadata from app markers in a JPEG file
  void jpeg_read_metadata(jpeg_decompress_struct* dinfo, Image::ptr img);

  //! Read an ICC profile from APP2 markers in a JPEG file
  CMS::Profile::ptr jpeg_read_profile(jpeg_decompress_struct* dinfo, Destination::ptr dest);

  //! Write an ICC profile into APP2 markers in a JPEG file
  void jpeg_write_profile(jpeg_compress_struct* cinfo, unsigned char *data, unsigned int size);

} // namespace PhotoFinish


#endif // __JPEG_HH__
