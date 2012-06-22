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
#include <errno.h>
#include <png.h>
#include <zlib.h>
#include <time.h>
#include <omp.h>
#include <lcms2.h>
#include "ImageFile.hh"
#include "Image.hh"

PNGFile::PNGFile(const char* filepath) :
  _ImageFile(filepath),
  _bit_depth(8),
  _profile(NULL),
  _intent(INTENT_PERCEPTUAL)
{}

PNGFile::~PNGFile() {
  if (_filepath != NULL)
    free((void*)_filepath);
  cmsCloseProfile(_profile);
}

bool PNGFile::set_bit_depth(int bit_depth) {
  if ((bit_depth == 8) || (bit_depth == 16)) {
    _bit_depth = bit_depth;
    return true;
  }
  return false;
}

bool PNGFile::set_profile(cmsHPROFILE profile, cmsUInt32Number intent) {
  _profile = profile;
  _intent = intent;
  return true;
}

Image& PNGFile::read(void) {
  fprintf(stderr, "Opening file \"%s\"...\n", _filepath);
  FILE *fp = fopen(_filepath, "r");
  if (!fp) {
    fprintf(stderr, "Could not open file \"%s\": %s\n", _filepath, strerror(errno));
    exit(1);
  }

  {
    unsigned char header[8];
    //    fprintf(stderr, "Reading header...\n");
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)) {
      fprintf(stderr, "File \"%s\" is not a PNG file.\n", _filepath);
      exit(2);
    }
  }

  //  fprintf(stderr, "Creating read structure...\n");
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					   NULL, NULL, NULL);
  if (!png) {
    fprintf(stderr, "Could not initialize PNG read structure.\n");
    exit(3);
  }

  //  fprintf(stderr, "Creating info structure...\n");
  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_read_struct(&png, (png_infopp)NULL, (png_infopp)NULL);
    exit(3);
  }

  //  fprintf(stderr, "Setting jump point...\n");
  if (setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    fprintf(stderr, "something went wrong reading the PNG.\n");
    exit(4);
  }

  //  fprintf(stderr, "Initialising PNG IO...\n");
  png_init_io(png, fp);

  png_set_sig_bytes(png, 8);
  png_set_gamma(png, 1.0, 1.0);
  png_set_alpha_mode(png, PNG_ALPHA_PNG, 1.0);

  fprintf(stderr, "Reading PNG image...\n");
  png_read_png(png, info, PNG_TRANSFORM_PACKING | PNG_TRANSFORM_SWAP_ENDIAN, NULL);
  //  fprintf(stderr, "Getting rows...\n");
  png_bytepp png_rows = png_get_rows(png, info);

  //  fprintf(stderr, "Getting the header information...\n");
  unsigned int width, height;
  int bit_depth, colour_type;
  png_get_IHDR(png, info, &width, &height, &bit_depth, &colour_type, NULL, NULL, NULL);
  fprintf(stderr, "%dx%d, %d bpp, type %d.\n", width, height, bit_depth, colour_type);

  Image *img = new Image(width, height);

  cmsUInt32Number cmsType;
  switch (colour_type) {
  case PNG_COLOR_TYPE_GRAY:
    cmsType = COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(bit_depth >> 3);
    break;
  case PNG_COLOR_TYPE_RGB:
    cmsType = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(bit_depth >> 3);
    break;
  default:
    fprintf(stderr, "unsupported PNG colour type %d\n", colour_type);
    exit(1);
  }

  cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
  cmsHPROFILE profile = NULL;

  if (png_get_valid(png, info, PNG_INFO_iCCP)) {
    fprintf(stderr, "Image has iCCP chunk.\n");
    char *profile_name;
    int compression_type;
    png_bytep profile_data;
    unsigned int profile_len;
    if (png_get_iCCP(png, info, &profile_name, &compression_type, &profile_data, &profile_len) == PNG_INFO_iCCP) {
      fprintf(stderr, "Loading ICC profile \"%s\" from file...\n", profile_name);
      profile = cmsOpenProfileFromMem(profile_data, profile_len);
    }
  }
  if (profile == NULL) {
    fprintf(stderr, "Using default sRGB profile...\n");
    profile = cmsCreate_sRGBProfile();
  }

  //  fprintf(stderr, "Creating colour transform...\n");
  cmsHTRANSFORM transform = cmsCreateTransform(profile, cmsType,
					       lab, TYPE_Lab_DBL,
					       INTENT_PERCEPTUAL, 0);
  cmsCloseProfile(lab);
  cmsCloseProfile(profile);

#pragma omp parallel
  {
#pragma omp master
    {
      fprintf(stderr, "Transforming image data to L*a*b* using %d threads...\n", omp_get_num_threads());
    }
  }
#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int y = 0; y < height; y++)
    cmsDoTransform(transform, png_rows[y], img->row(y), width);

  cmsDeleteTransform(transform);

  fprintf(stderr, "Done.\n");
  png_destroy_read_struct(&png, &info, NULL);
  fclose(fp);

  return *img;
}

bool PNGFile::write(Image& img) {
  fprintf(stderr, "Opening file \"%s\"...\n", _filepath);
  FILE *fp = fopen(_filepath, "wb");
  if (!fp) {
    fprintf(stderr, "Could not open file \"%s\": %s\n", _filepath, strerror(errno));
    return false;
  }

  //  fprintf(stderr, "Creating write structure...\n");
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
					    NULL, NULL, NULL);
  if (!png) {
    fprintf(stderr, "Could not initialize PNG write structure.\n");
    return false;
  }

  //  fprintf(stderr, "Creating info structure...\n");
  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_write_struct(&png, (png_infopp)NULL);
    return false;
  }

  //  fprintf(stderr, "Setting jump point...\n");
  if (setjmp(png_jmpbuf(png))) {
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    fprintf(stderr, "something went wrong reading the PNG.\n");
    return false;
  }

  //  fprintf(stderr, "Initialising PNG IO...\n");
  png_init_io(png, fp);

  fprintf(stderr, "writing header for %dx%d %d-bit RGB PNG image...\n", img.width(), img.height(), _bit_depth);
  png_set_IHDR(png, info,
	       img.width(), img.height(), _bit_depth, PNG_COLOR_TYPE_RGB,
	       PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_set_filter(png, 0, PNG_ALL_FILTERS);
  png_set_compression_level(png, Z_BEST_COMPRESSION);

  if (_profile == NULL) {
    fprintf(stderr, "Using default sRGB profile...\n");
    _profile = cmsCreate_sRGBProfile();
    png_set_sRGB_gAMA_and_cHRM(png, info, _intent);
  } else {
    cmsUInt32Number len;
    len = cmsGetProfileInfoASCII(_profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, NULL, 0);
    if (len > 0) {
      char *profile_name = (char *)malloc(len);
      cmsGetProfileInfoASCII(_profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, profile_name, len);
      cmsSaveProfileToMem(_profile, NULL, &len);
      if (len > 0) {
	png_bytep profile_data = (png_bytep)malloc(len);
	if (cmsSaveProfileToMem(_profile, profile_data, &len)) {
	  fprintf(stderr, "Embedding profile \"%s\" (%d bytes)...\n", profile_name, len);
	  png_set_iCCP(png, info, profile_name, 0, profile_data, len);
	}
      }
    }
  }

  {
    time_t t = time(NULL);
    if (t > 0) {
      png_time ptime;
      png_convert_from_time_t(&ptime, t);
      fprintf(stderr, "Adding time chunk...\n");
      png_set_tIME(png, info, &ptime);
    }
  }

  png_bytepp png_rows = (png_bytepp)malloc(img.height() * sizeof(png_bytep));

  for (unsigned int y = 0; y < img.height(); y++)
    png_rows[y] = NULL;  /* security precaution */

  for (unsigned int y = 0; y < img.height(); y++)
    png_rows[y] = (png_bytep)malloc(img.width() * 3 * (_bit_depth >> 3));

  png_set_rows(png, info, png_rows);

  cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
  //  fprintf(stderr, "Creating colour transform...\n");
  cmsUInt32Number format = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(_bit_depth >> 3);
  cmsHTRANSFORM transform = cmsCreateTransform(lab, TYPE_Lab_DBL,
					       _profile, format,
					       _intent, 0);
  cmsCloseProfile(lab);

#pragma omp parallel
  {
#pragma omp master
    {
      fprintf(stderr, "Transforming image data from L*a*b* using %d threads...\n", omp_get_num_threads());
    }
  }
#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int y = 0; y < img.height(); y++)
    cmsDoTransform(transform, img.row(y), png_rows[y], img.width());

  cmsDeleteTransform(transform);

  fprintf(stderr, "Writing PNG image data...\n");
  png_write_png(png, info, PNG_TRANSFORM_SWAP_ENDIAN, NULL);

  fprintf(stderr, "Done.\n");
  fclose(fp);

  return true;
}

