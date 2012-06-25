#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <omp.h>
#include <lcms2.h>
#include "ImageFile.hh"
#include "Image.hh"

JPEGFile::JPEGFile(const char* filepath) :
  _ImageFile(filepath)
{}

bool JPEGFile::set_bit_depth(int bit_depth) {
  if (bit_depth == 8)
    return true;
  return false;
}

bool JPEGFile::set_profile(cmsHPROFILE profile, cmsUInt32Number intent) {
  return true;
}

Image* JPEGFile::read(void) {
  return NULL;
}

bool JPEGFile::write(Image* img) {
  jpeg_compress_struct cinfo;
  jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  fprintf(stderr, "Opening file \"%s\"...\n", _filepath);
  FILE *fp = fopen(_filepath, "wb");
  if (!fp) {
    fprintf(stderr, "can't open file \"%s\"\n", _filepath);
    return false;
  }
  jpeg_stdio_dest(&cinfo, fp);

  cinfo.image_width = img->width();
  cinfo.image_height = img->height();
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, 95, TRUE);

  cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
  cmsHPROFILE sRGB = cmsCreate_sRGBProfile();
  cmsHTRANSFORM transform = cmsCreateTransform(lab, IMAGE_TYPE,
					       sRGB, TYPE_RGB_8,
					       INTENT_PERCEPTUAL, 0);
  cmsCloseProfile(lab);
  cmsCloseProfile(sRGB);

  JSAMPROW row[1];
  row[0] = (JSAMPROW)malloc(img->width() * 3 * sizeof(JSAMPLE));

  fprintf(stderr, "Writing JPEG file...\n");
  jpeg_start_compress(&cinfo, TRUE);
  while (cinfo.next_scanline < cinfo.image_height) {
    cmsDoTransform(transform, img->row(cinfo.next_scanline), row[0], img->width());
    jpeg_write_scanlines(&cinfo, row, 1);
  }

  jpeg_finish_compress(&cinfo);
  fclose(fp);
  jpeg_destroy_compress(&cinfo);

  return true;
}

