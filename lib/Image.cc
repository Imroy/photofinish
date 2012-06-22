#include "Image.hh"
#include <errno.h>
#include <png.h>
#include <zlib.h>
#include <lcms2.h>
#include <time.h>
#include <omp.h>

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

  //  fprintf(stderr, "Creating read structure...\n");
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					   (png_voidp)this, image_png_error, image_png_warning);
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

  fprintf(stderr, "Getting the header information...\n");
  int bit_depth, colour_type;
  png_get_IHDR(png, info, &_width, &_height, &bit_depth, &colour_type, NULL, NULL, NULL);
  fprintf(stderr, "%dx%d, %d bpp, type %d.\n", _width, _height, bit_depth, colour_type);

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

  rowdata = (double**)malloc(_height * sizeof(double*));
  for (unsigned int y = 0; y < _height; y++)
    rowdata[y] = (double*)malloc(_width * 3 * sizeof(double));

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

  fprintf(stderr, "Creating colour transform...\n");
  cmsHTRANSFORM transform = cmsCreateTransform(profile, cmsType,
					       lab, TYPE_Lab_DBL,
					       INTENT_PERCEPTUAL, 0);
  cmsCloseProfile(lab);
  cmsCloseProfile(profile);

  fprintf(stderr, "Transforming image data...\n");
#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int y = 0; y < _height; y++)
    cmsDoTransform(transform, png_rows[y], rowdata[y], _width);

  cmsDeleteTransform(transform);

  fprintf(stderr, "Done.\n");
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

#define sqr(x) ((x) * (x))

inline double lanczos(double a, double ra, double x) {
  if (fabs(x) < 1e-6)
    return 1.0;
  double pix = M_PI * x;
  return (a * sin(pix) * sin(pix * ra)) / (sqr(M_PI) * sqr(x));
}

Image& Image::resize(double nw, double nh, double a) {
  double wscale = nw / _width;
  double hscale = nh / _height;
  if (nw < 0) {
    wscale = hscale;
    nw = _width * wscale;
  } else if (nh < 0) {
    hscale = wscale;
    nh = _height * hscale;
  }
  double wdivide = 1.0 / wscale;
  double hdivide = 1.0 / hscale;
  unsigned int nwi = ceil(nw);
  unsigned int nhi = ceil(nh);
  double ra = 1.0 / a;

  fprintf(stderr, "nw=%f, nh=%f, nwi=%d, nhi=%d, wscale=%f, hscale=%f, wdivide=%f, hdivide=%f\n",
	  nw, nh, nwi, nhi, wscale, hscale, wdivide, hdivide);

  Image *ni = new Image(nwi, nhi);

#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int ny = 0; ny < nhi; ny++) {
    double ry = ny * hdivide;
    unsigned start_y, end_y = ceil(ry + a);
    if (floor(ry - a) < 0)
      start_y = 0;
    else
      start_y = floor(ry - a);
    if (end_y > _height)
      end_y = _height;
    double *npix = ni->row(ny);

    for (unsigned int nx = 0; nx < nwi; nx++) {
      double rx = nx * wdivide;
      unsigned start_x, end_x = ceil(rx + a);
      if (floor(rx - a) < 0)
	start_x = 0;
      else
	start_x = floor(rx - a);
      if (end_x > _width)
	end_x = _width;

      if ((nx == 500) && (ny == 500))
	fprintf(stderr, "nx=%d, ny=%d, rx=%f, ry=%f, start_x=%d, end_x=%d, start_y=%d, end_y=%d\n",
		nx, ny, rx, ry, start_x, end_x, start_y, end_y);

      double total_weight = 0, total_value[3] = { 0, 0, 0 };
      for (unsigned int y = start_y; y < end_y; y++) {
	double dy = y - ry;
	if ((dy < -a) || (dy > a))
	  continue;
	double weight_y = lanczos(a, ra, dy);
	double *pix = this->at(start_x, y);
	for (unsigned int x = start_x; x < end_x; x++) {
	  double dx = x - rx;
	  if ((dx < -a) || (dx > a))
	    continue;
	  double weight = weight_y * lanczos(a, ra, dx);
	  if ((nx == 500) && (ny == 500))
	    fprintf(stderr, "x=%d, y=%d, dx=%f, dy=%f, weight_y=%f, weight=%f\n",
		    x, y, dx, dy, weight_y, weight);
	  total_weight += weight;
	  total_value[0] += weight * *pix++;
	  total_value[1] += weight * *pix++;
	  total_value[2] += weight * *pix++;
	}
      }
      if (total_weight > 1e-6) {
	double rweight = 1.0 / total_weight;
	total_value[0] *= rweight;
	total_value[1] *= rweight;
	total_value[2] *= rweight;
      }
      *npix++ = total_value[0];
      *npix++ = total_value[1];
      *npix++ = total_value[2];
    }
  }

  return *ni;
}

void Image::write_png(const char* filepath, int bit_depth, cmsHPROFILE profile, cmsUInt32Number intent) {
  fprintf(stderr, "Opening file \"%s\"...\n", filepath);
  FILE *fp = fopen(filepath, "wb");
  if (!fp) {
    fprintf(stderr, "Could not open file \"%s\": %s\n", filepath, strerror(errno));
    exit(1);
  }
  
  //  fprintf(stderr, "Creating write structure...\n");
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
					   (png_voidp)this, image_png_error, image_png_warning);
  if (!png) {
    fprintf(stderr, "Could not initialize PNG write structure.\n");
    exit(3);
  }

  //  fprintf(stderr, "Creating info structure...\n");
  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_write_struct(&png, (png_infopp)NULL);
    exit(3);
  }

  //  fprintf(stderr, "Setting jump point...\n");
  if (setjmp(png_jmpbuf(png))) {
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    fprintf(stderr, "something went wrong reading the PNG.\n");
    exit(4);
  }

  //  fprintf(stderr, "Initialising PNG IO...\n");
  png_init_io(png, fp);

  fprintf(stderr, "writing header for %dx%d %d-bit RGB PNG image...\n", _width, _height, bit_depth);
  png_set_IHDR(png, info,
	       _width, _height, bit_depth, PNG_COLOR_TYPE_RGB,
	       PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_set_filter(png, 0, PNG_ALL_FILTERS);
  png_set_compression_level(png, Z_BEST_COMPRESSION);

  if (profile == NULL) {
    fprintf(stderr, "Using default sRGB profile...\n");
    profile = cmsCreate_sRGBProfile();
    png_set_sRGB_gAMA_and_cHRM(png, info, intent);
  } else {
    cmsUInt32Number len;
    len = cmsGetProfileInfoASCII(profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, NULL, 0);
    if (len > 0) {
      char *profile_name = (char *)malloc(len);
      cmsGetProfileInfoASCII(profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, profile_name, len);
      cmsSaveProfileToMem(profile, NULL, &len);
      if (len > 0) {
	png_bytep profile_data = (png_bytep)malloc(len);
	if (cmsSaveProfileToMem(profile, profile_data, &len)) {
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

  png_bytepp png_rows = (png_bytepp)malloc(_height * sizeof(png_bytep));

  for (unsigned int y = 0; y < _height; y++)
    png_rows[y] = NULL;  /* security precaution */

  for (unsigned int y = 0; y < _height; y++)
    png_rows[y] = (png_bytep)malloc(_width * 3 * (bit_depth >> 3));

  png_set_rows(png, info, png_rows);

  cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
  fprintf(stderr, "Creating colour transform...\n");
  cmsUInt32Number format = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(bit_depth >> 3);
  fprintf(stderr, "format=A%d O%d T%d Y%d F%d P%d X%d S%d E%d C%d B%d\n",
	  T_FLOAT(format),
	  T_OPTIMIZED(format),
	  T_COLORSPACE(format),
	  T_SWAPFIRST(format),
	  T_FLAVOR(format),
	  T_PLANAR(format),
	  T_ENDIAN16(format),
	  T_DOSWAP(format),
	  T_EXTRA(format),
	  T_CHANNELS(format),
	  T_BYTES(format));
  cmsHTRANSFORM transform = cmsCreateTransform(lab, TYPE_Lab_DBL,
					       profile, format,				       
					       intent, 0);
  cmsCloseProfile(lab);
  cmsCloseProfile(profile);

  fprintf(stderr, "Transforming image data...\n");
#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int y = 0; y < _height; y++)
    cmsDoTransform(transform, rowdata[y], png_rows[y], _width);

  cmsDeleteTransform(transform);

  fprintf(stderr, "Writing PNG image data...\n");
  png_write_png(png, info, PNG_TRANSFORM_SWAP_ENDIAN, NULL);

  fprintf(stderr, "Done.\n");
  fclose(fp);
}

void Image::write_jpeg(const char* filepath, cmsHPROFILE profile, cmsUInt32Number intent) {
}
