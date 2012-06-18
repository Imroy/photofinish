#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <png.h>
#include <lcms2.h>

void image_png_error(png_structp png, png_const_charp msg) {
  fprintf(stderr, "%s\n", msg);
  exit(5);
}

void image_png_warning(png_structp png, png_const_charp msg) {
  fprintf(stderr, "%s\n", msg);
}

template <typename P>
Image<P>::Image(const char* filepath) :
  _width(0),
  _height(0),
  _channels(0),
  rowdata(NULL),
  _profile(NULL)
{
  FILE *fp = fopen(filepath, "r");
  if (!fp) {
    fprintf(stderr, "Could not open file \"%s\": %s\n", filepath, strerror(errno));
    exit(1);
  }

  {
    unsigned char header[8];
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)) {
      fprintf(stderr, "File \"%s\" is not a PNG file.\n", filepath);
      exit(2);
    }
  }

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					   (png_voidp)this, image_png_error, image_png_warning);
  if (!png) {
    fprintf(stderr, "Could not initialize PNG read structure.\n");
    exit(3);
  }

  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_read_struct(&png, (png_infopp)NULL, (png_infopp)NULL);
    exit(3);
  }

  /*
  png_infop end_info = png_create_info_struct(png);
  if (!end_info) {
    png_destroy_read_struct(&png, &info, (png_infopp)NULL);
    exit(3);
  }
  */

  if (setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    fprintf(stderr, "something went wrong reading the PNG.\n");
    exit(4);
  }

  png_init_io(png, fp);
  png_set_sig_bytes(png, 8);
  png_set_gamma(png, 1.0, 1.0);
  png_set_alpha_mode(png, PNG_ALPHA_PNG, 1.0);

  png_read_png(png, info, PNG_TRANSFORM_PACKING, NULL);
  png_bytepp png_rows = png_get_rows(png, info);

  png_read_end(png, NULL);

  int bit_depth, colour_type;
  png_get_IHDR(png, info, &_width, &_height, &bit_depth, &colour_type, NULL, NULL, NULL);

  rowdata = (P**)malloc(_height * sizeof(P*));
  for (unsigned int y = 0; y < _height; y++)
    rowdata[y] = (P*)malloc(_width * _channels * sizeof(P));

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

  for (unsigned int y = 0; y < _height; y++) {
    P *dst = rowdata[y];
    if (bit_depth <= 8) {
      cmsUInt8Number *src = png_rows[y];
      for (unsigned int x = 0; x < _width * channels; x++)
	*dst++ = *src++;
    } else {
      cmsUInt16Number *src = (cmsUInt16Number*)png_rows[y];
      for (unsigned int x = 0; x < _width * channels; x++)
	*dst++ = *src++;
    }
  }    

  if (png_get_valid(png, info, PNG_INFO_iCCP)) {
    char *profile_name;
    png_bytep profile;
    unsigned int profile_len;
    if (png_get_iCCP(png, info, &profile_name, NULL, &profile, &profile_len) == PNG_INFO_iCCP) {
      fprintf(stderr, "Image: profile=%8p, profile_len=%d.\n", profile, profile_len);
      _profile = cmsOpenProfileFromMem(profile, profile_len);
    }
  }

  png_destroy_read_struct(&png, &info, NULL);
  fclose(fp);
}
