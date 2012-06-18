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
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <png.h>
#include <lcms2.h>

template <typename P>
Image<P>::Image(const char* filepath) {
  FILE *fp = fopen(filepath, "r");
  if (!fp) {
    fprintf(stderr, "Could not open file \"%s\": %s\n", filepath, strerror(errno));
    exit(1);
  }

  unsigned char header[8];
  fread(header, 1, 8, fp);
  int not_png = png_sig_cmp(header, 0, 8);
  if (not_png) {
    fprintf(stderr, "File \"%s\" is not a PNG file.\n", filepath);
    exit(2);
  }

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) {
    fprintf(stderr, "Could not initialize PNG read structure.\n");
    exit(3);
  }

  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_read_struct(&png, (png_infopp)NULL, (png_infopp)NULL);
    exit(3);
  }

  png_infop end_info = png_create_info_struct(png);
  if (!end_info) {
    png_destroy_read_struct(&png, &info, (png_infopp)NULL);
    exit(3);
  }

  if (setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png, &info, &end_info);
    fclose(fp);
    fprintf(stderr, "something went wrong reading the PNG.\n");
    exit(1);
  }

  png_init_io(png, fp);
  png_set_sig_bytes(png, 8);
  png_read_png(png, info, PNG_TRANSFORM_PACKING, NULL);

  png_bytepp png_rows = png_get_rows(png, info);

  int bit_depth, colour_type;
  png_get_IHDR(png, info, &_width, &_height,
               &bit_depth, &colour_type, NULL, NULL, NULL);

  char channels;
  switch (colour_type) {
  case PNG_COLOR_TYPE_GRAY:
    _cmsType = COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(bit_depth / 8);
    channels = 1;
    break;
  case PNG_COLOR_TYPE_GRAY_ALPHA:
    _cmsType = COLORSPACE_SH(PT_GRAY) | EXTRA_SH(1) | CHANNELS_SH(1) | BYTES_SH(bit_depth / 8);
    channels = 2;
    break;
  case PNG_COLOR_TYPE_RGB:
    _cmsType = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(bit_depth / 8);
    channels = 3;
    break;
  case PNG_COLOR_TYPE_RGB_ALPHA:
    _cmsType = COLORSPACE_SH(PT_RGB) | EXTRA_SH(1) | CHANNELS_SH(3) | BYTES_SH(bit_depth / 8);
    channels = 4;
    break;
  default:
    fprintf(stderr, "unknown PNG colour type %d\n", colour_type);
    exit(1);
  }

  if (std::is_floating_point<P>::value) {
    for (unsigned int y = 0; y < _height; y++) {
      P *dst = rowdata[y];
      if (T_BYTES(_cmsType) == 1) {
	cmsUInt8Number *src = png_rows[y];
	for (unsigned int x = 0; x < _width * channels; x++)
	  *dst++ = *src++;
      } else {
	cmsUInt16Number *src = (cmsUInt16Number*)png_rows[y];
	for (unsigned int x = 0; x < _width * channels; x++)
	  *dst++ = *src++;
      }
    }    
  } else {
    for (unsigned int y = 0; y < _height; y++)
      memcpy((void*)rowdata[y], (void*)png_rows, _width * channels * T_BYTES(_cmsType));
  }

  char *profile_name;
  png_bytep profile;
  unsigned int profile_len;
  png_get_iCCP(png, info, &profile_name, NULL, &profile, &profile_len);
  
}
