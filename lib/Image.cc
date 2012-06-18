#include "Image.hh"
#include <errno.h>
#include <png.h>
#include <lcms2.h>

Image::Image(unsigned int w, unsigned int h) :
  _width(w),
  _height(h),
  rowdata(NULL)
{
  rowdata = (double**)malloc(_height * sizeof(double*));
  for (unsigned int y = 0; y < _height; y++)
    rowdata[y] = (double*)malloc(_width * 3 * sizeof(double));
}

Image::Image(Image& other) :
  _width(other._width),
  _height(other._height),
  rowdata(NULL)
{
  rowdata = (double**)malloc(_height * sizeof(double*));
  for (unsigned int y = 0; y < _height; y++) {
    rowdata[y] = (double*)malloc(_width * 3 * sizeof(double));
    memcpy(rowdata[y], other.rowdata[y], _width * 3 * sizeof(double));
  }
}

void image_png_error(png_structp png, png_const_charp msg) {
  fprintf(stderr, "%s\n", msg);
  exit(5);
}

void image_png_warning(png_structp png, png_const_charp msg) {
  fprintf(stderr, "%s\n", msg);
}

Image::Image(const char* filepath) :
  _width(0),
  _height(0),
  rowdata(NULL)
{
  fprintf(stderr, "Opening file \"%s\"...\n", filepath);
  FILE *fp = fopen(filepath, "r");
  if (!fp) {
    fprintf(stderr, "Could not open file \"%s\": %s\n", filepath, strerror(errno));
    exit(1);
  }

  {
    unsigned char header[8];
    fprintf(stderr, "Reading header...\n");
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)) {
      fprintf(stderr, "File \"%s\" is not a PNG file.\n", filepath);
      exit(2);
    }
  }

  fprintf(stderr, "Creating read structure...\n");
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					   (png_voidp)this, image_png_error, image_png_warning);
  if (!png) {
    fprintf(stderr, "Could not initialize PNG read structure.\n");
    exit(3);
  }

  fprintf(stderr, "Creating info structure...\n");
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

  fprintf(stderr, "Setting jump point...\n");
  if (setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    fprintf(stderr, "something went wrong reading the PNG.\n");
    exit(4);
  }

  fprintf(stderr, "Initialising PNG IO...\n");
  png_init_io(png, fp);

  png_set_sig_bytes(png, 8);
  png_set_gamma(png, 1.0, 1.0);
  png_set_alpha_mode(png, PNG_ALPHA_PNG, 1.0);

  fprintf(stderr, "Reading PNG image...\n");
  png_read_png(png, info, PNG_TRANSFORM_PACKING, NULL);
  fprintf(stderr, "Getting rows...\n");
  png_bytepp png_rows = png_get_rows(png, info);

  /*
  fprintf(stderr, "Ending the read process.\n");
  png_read_end(png, NULL);
  */

  fprintf(stderr, "Getting the header information...\n");
  int bit_depth, colour_type;
  png_get_IHDR(png, info, &_width, &_height, &bit_depth, &colour_type, NULL, NULL, NULL);
  fprintf(stderr, "%dx%d, %d bpp, type %d.\n", _width, _height, bit_depth, colour_type);

  cmsUInt32Number cmsType;
  switch (colour_type) {
  case PNG_COLOR_TYPE_GRAY:
    cmsType = COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(bit_depth / 8);
    break;
  case PNG_COLOR_TYPE_RGB:
    cmsType = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(bit_depth / 8);
    break;
  default:
    fprintf(stderr, "unknown PNG colour type %d\n", colour_type);
    exit(1);
  }

  rowdata = (double**)malloc(_height * sizeof(double*));
  for (unsigned int y = 0; y < _height; y++)
    rowdata[y] = (double*)malloc(_width * 3 * sizeof(double));

  cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
  cmsHPROFILE profile = cmsCreate_sRGBProfile();

  if (png_get_valid(png, info, PNG_INFO_iCCP)) {
    char *profile_name;
    png_bytep profile_data;
    unsigned int profile_len;
    if (png_get_iCCP(png, info, &profile_name, NULL, &profile_data, &profile_len) == PNG_INFO_iCCP) {
      cmsCloseProfile(profile);
      profile = cmsOpenProfileFromMem(profile_data, profile_len);
    }
  }

  cmsHTRANSFORM transform = cmsCreateTransform(profile, cmsType,
					       lab, TYPE_Lab_DBL,
					       INTENT_PERCEPTUAL, 0);
  cmsCloseProfile(lab);
  cmsCloseProfile(profile);

  for (unsigned int y = 0; y < _height; y++)
    cmsDoTransform(transform, png_rows[y], rowdata[y], _width);

  cmsDeleteTransform(transform);

  png_destroy_read_struct(&png, &info, NULL);
  fclose(fp);
}

Image::~Image() {
  if (rowdata != NULL) {
    for (unsigned int y = 0; y < _height; y++)
      free(rowdata[y]);
    free(rowdata);
  }
}
