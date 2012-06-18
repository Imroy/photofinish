#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <png.h>

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

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fprintf(stderr, "Could not initialize PNG read structure.\n");
    exit(3);
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    exit(3);
  }

  
}
