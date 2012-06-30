#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <omp.h>
#include <lcms2.h>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>
#include "ImageFile.hh"
#include "Image.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  JPEGFile::JPEGFile(const fs::path filepath) :
    _ImageFile(filepath)
  {}

  const Image& JPEGFile::read(void) {
    throw Unimplemented("JPEGFile", "read()");
  }

  struct callback_state {
    JOCTET *buffer;
    std::ofstream *fb;
    size_t buffer_size;
  };

  static void ofstream_init_destination(j_compress_ptr cinfo) {
    jpeg_destination_mgr *dmgr = (jpeg_destination_mgr*)(cinfo->dest);
    callback_state *cs = (callback_state*)(cinfo->client_data);
    cs->buffer = (JOCTET*)malloc(cs->buffer_size);
    if (cs->buffer == NULL)
      throw MemAllocError("Out of memory?");

    dmgr->next_output_byte = cs->buffer;
    dmgr->free_in_buffer = cs->buffer_size;
  }

  static boolean ofstream_empty_output_buffer(j_compress_ptr cinfo) {
    jpeg_destination_mgr *dmgr = (jpeg_destination_mgr*)(cinfo->dest);
    callback_state *cs = (callback_state*)(cinfo->client_data);
    cs->fb->write((char*)cs->buffer, cs->buffer_size);
    dmgr->next_output_byte = cs->buffer;
    dmgr->free_in_buffer = cs->buffer_size;
    return 1;
  }

  static void ofstream_term_destination(j_compress_ptr cinfo) {
    jpeg_destination_mgr *dmgr = (jpeg_destination_mgr*)(cinfo->dest);
    callback_state *cs = (callback_state*)(cinfo->client_data);
    cs->fb->write((char*)cs->buffer, cs->buffer_size - dmgr->free_in_buffer);
    free(cs->buffer);
    cs->buffer = NULL;
    dmgr->free_in_buffer = 0;
  }

  void JPEGFile::write(const Image& img, const Destination &d) {
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    fprintf(stderr, "Opening file \"%s\"...\n", _filepath.string().c_str());
    fs::ofstream fb(_filepath, std::ios_base::out);

    callback_state cs;
    cs.fb = &fb;
    cs.buffer_size = 1048576;
    cinfo.client_data = (void*)&cs;

    jpeg_destination_mgr dmgr;
    dmgr.init_destination = ofstream_init_destination;
    dmgr.empty_output_buffer = ofstream_empty_output_buffer;
    dmgr.term_destination = ofstream_term_destination;
    cinfo.dest = &dmgr;

    cinfo.image_width = img.width();
    cinfo.image_height = img.height();

    cmsHPROFILE profile = NULL;
    cmsUInt32Number cmsType;
    if (img.is_greyscale()) {
      fprintf(stderr, "Using default greyscale profile...\n");
      cmsToneCurve *gamma = cmsBuildGamma(NULL, 2.2);
      profile = cmsCreateGrayProfile(cmsD50_xyY(), gamma);
      cmsFreeToneCurve(gamma);
      cmsType = COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(1);
      cinfo.input_components = 1;
      cinfo.in_color_space = JCS_GRAYSCALE;
    } else {
      fprintf(stderr, "Using default sRGB profile...\n");
      profile = cmsCreate_sRGBProfile();
      cmsType = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(1);
      cinfo.input_components = 3;
      cinfo.in_color_space = JCS_RGB;
    }

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
    cmsHTRANSFORM transform = cmsCreateTransform(lab, IMAGE_TYPE,
						 profile, cmsType,
						 INTENT_PERCEPTUAL, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    JSAMPROW row[1];
    row[0] = (JSAMPROW)malloc(img.width() * cinfo.input_components * sizeof(JSAMPLE));

    fprintf(stderr, "Writing JPEG file...\n");
    jpeg_start_compress(&cinfo, TRUE);
    while (cinfo.next_scanline < cinfo.image_height) {
      cmsDoTransform(transform, img.row(cinfo.next_scanline), row[0], img.width());
      jpeg_write_scanlines(&cinfo, row, 1);
    }

    jpeg_finish_compress(&cinfo);
    fb.close();
    jpeg_destroy_compress(&cinfo);
  }

}
