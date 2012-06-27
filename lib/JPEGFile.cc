#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <omp.h>
#include <lcms2.h>
#include "ImageFile.hh"
#include "Image.hh"

namespace PhotoFinish {

  JPEGFile::JPEGFile(const std::string filepath) :
    _ImageFile(filepath)
  {}

  Image* JPEGFile::read(void) {
    return NULL;
  }

  bool JPEGFile::write(Image* img, const Destination &d) {
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    fprintf(stderr, "Opening file \"%s\"...\n", _filepath.c_str());
    FILE *fp = fopen(_filepath.c_str(), "wb");
    if (!fp) {
      fprintf(stderr, "can't open file \"%s\"\n", _filepath.c_str());
      return false;
    }
    jpeg_stdio_dest(&cinfo, fp);

    cinfo.image_width = img->width();
    cinfo.image_height = img->height();
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    if (d.has_jpeg()) {
      D_JPEG jpeg = d.jpeg();
      fprintf(stderr, "JPEGFile: Got quality of %d.\n", jpeg.quality());
      jpeg_set_quality(&cinfo, jpeg.quality(), TRUE);
      if (jpeg.progressive()) {
	fprintf(stderr, "JPEGFile: Progressive.\n");
	jpeg_simple_progression(&cinfo);	// TODO: Custom scan sequence
      }
      fprintf(stderr, "JPEGFile: %dx%d chroma sub-sampling.\n", jpeg.sample_h(), jpeg.sample_v());
      cinfo.comp_info[0].h_samp_factor = jpeg.sample_h();
      cinfo.comp_info[0].v_samp_factor = jpeg.sample_v();
    } else {
      jpeg_set_quality(&cinfo, 95, TRUE);
    }

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

}
