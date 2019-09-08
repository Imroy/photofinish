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
#pragma once

#include <openjpeg.h>

namespace PhotoFinish {

  //! Error callback for OpenJPEG - throw a LibraryError exception
  void error_callback(const char* msg, void* client_data);

  //! Warning callback for OpenJPEG - print the message to STDERR
  void warning_callback(const char* msg, void* client_data);

  //! Info callback for OpenJPEG - print the indented message to STDERR
  void info_callback(const char* msg, void* client_data);

  //! Read a row of image data from OpenJPEG's planar integer components into an LCMS2-compatible single array
  template <typename T>
  inline void read_planar(unsigned int width, unsigned char channels, opj_image_t* image, T* row, unsigned int y) {
    T *out = row;
    unsigned int index_start = y * width;
    for (unsigned char c = 0; c < channels; c++) {
      unsigned int index = index_start;
      for (unsigned int x = 0; x < width; x++, out++, index++)
	*out = image->comps[c].data[index];
    }
  }

  //! Read a row of planar pixel data into OpenJPEG's planar components
  template <typename T>
  void write_planar(unsigned int width, unsigned char channels, T* row, opj_image_t* image, unsigned int y) {
    T *in = row;
    unsigned int index_start = y * width;
    for (unsigned char c = 0; c < channels; c++) {
      unsigned int index = index_start;
      for (unsigned int x = 0; x < width; x++, index++, in++)
	image->comps[c].data[index] = *in;
    }
  }

  //! Read a row of packed pixel data into OpenJPEG's planar components
  template <typename T>
  void write_packed(unsigned int width, unsigned char channels, T* row, opj_image_t* image, unsigned int y) {
    T *in = row;
    unsigned int index = y * width;
    for (unsigned int x = 0; x < width; x++, index++)
      for (unsigned char c = 0; c < channels; c++, in++)
	image->comps[c].data[index] = *in;
  }

} // namespace PhotoFinish
