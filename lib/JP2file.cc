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
#include <openjpeg.h>
#include <lcms2.h>
#include "ImageFile.hh"
#include "TransformQueue.hh"
#include "Ditherer.hh"
#include "Exception.hh"

namespace PhotoFinish {

  JP2file::JP2file(const fs::path filepath) :
    ImageFile(filepath)
  {}

  void error_callback(const char* msg, void* client_data) {
    throw LibraryError("OpenJPEG", msg);
  }

  void warning_callback(const char* msg, void* client_data) {
    ((char*)msg)[strlen(msg) - 1] = 0;
    std::cerr << "** OpenJPEG:" << msg << " **" << std::endl;
  }

  void info_callback(const char* msg, void* client_data) {
    ((char*)msg)[strlen(msg) - 1] = 0;
    std::cerr << "\tOpenJPEG:" << msg << std::endl;
  }

  Image::ptr JP2file::read(Destination::ptr dest) const {
    throw Unimplemented("JPEGFile", "read()");
  }

  void JP2file::write(Image::ptr img, Destination::ptr dest) const {
    opj_event_mgr_t event_mgr;
    memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
    event_mgr.error_handler = error_callback;
    event_mgr.warning_handler = warning_callback;
    event_mgr.info_handler = info_callback;

    opj_cparameters_t parameters;
    opj_set_default_encoder_parameters(&parameters);

    parameters.tile_size_on = 0;

    OPJ_COLOR_SPACE colour_space;
    cmsUInt32Number cmsTempType = BYTES_SH(2);
    if (img->is_colour()) {
      colour_space = CLRSPC_SRGB;
      cmsTempType |= COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3);
    } else {
      colour_space = CLRSPC_GRAY;
      cmsTempType |= COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1);
    }

    int depth = 8;	// Default value
    if (dest->depth().defined())
      depth = dest->depth();

    opj_image_cmptparm_t *components = (opj_image_cmptparm_t*)malloc(T_CHANNELS(cmsTempType) * sizeof(opj_image_cmptparm_t));
    memset(components, 0, T_CHANNELS(cmsTempType) * sizeof(opj_image_cmptparm_t));
    for (unsigned char i = 0; i < T_CHANNELS(cmsTempType); i++) {
      components[i].dx = parameters.subsampling_dx;
      components[i].dy = parameters.subsampling_dy;
      components[i].w = img->width();
      components[i].h = img->height();
      components[i].x0 = components[i].y0 = 0;
      components[i].prec = components[i].bpp = depth;
      components[i].sgnd = 0;
    }
    opj_image_t *jp2_image = opj_image_create(T_CHANNELS(cmsTempType), components, colour_space);

    if (jp2_image == NULL)
      return;

    cmsUInt32Number intent = INTENT_PERCEPTUAL;	// Default value
    if (dest->intent().defined())
      intent = dest->intent();

    cmsHPROFILE profile = this->default_profile(cmsTempType);
    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHTRANSFORM transform = cmsCreateTransform(lab, IMAGE_TYPE,
						 profile, cmsTempType,
						 intent, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    unsigned char **rows = (unsigned char**)malloc(img->height() * sizeof(unsigned char*));
    size_t channel_size = img->width() * (depth >> 3);
    size_t row_size = channel_size * T_CHANNELS(cmsTempType);
    for (unsigned int y = 0; y < img->height(); y++)
      rows[y] = (unsigned char*)malloc(row_size);

    transform_queue queue(dest, img, T_CHANNELS(cmsTempType), transform);
    if (depth == 8)
      for (unsigned int y = 0; y < img->height(); y++)
	queue.add(y);
    else
      for (unsigned int y = 0; y < img->height(); y++)
	queue.add(y, rows[y]);

#pragma omp parallel shared(queue)
    {
      int th_id = omp_get_thread_num();
      if (th_id == 0) {		// Master thread
	std::cerr << "\tTransforming image data from L*a*b* using " << omp_get_num_threads() << " threads." << std::endl;

	Ditherer ditherer(img->width(), T_CHANNELS(cmsTempType));

	for (unsigned int y = 0; y < img->height(); y++) {
	  // Process rows until the one we need becomes available, or the queue is empty
	  {
	    short unsigned int *row = queue.row(y);
	    while (!queue.empty() && (row == NULL)) {
	      queue.writer_process_row();
	      row = queue.row(y);
	    }

	    // If it's still not available, something has gone wrong
	    if (row == NULL) {
	      std::cerr << "** Oh crap (y=" << y << ", num_rows=" << queue.num_rows() << " **" << std::endl;
	      exit(2);
	    }

	    unsigned int index = y * img->width();
	    if (depth == 8) {
	      ditherer.dither(row, rows[y], y == img->height() - 1);
	      queue.free_row(y);

	      unsigned char *in = rows[y];
	      for (unsigned int x = 0; x < img->width(); x++, index++)
		for (unsigned char c = 0; c < T_CHANNELS(cmsTempType); c++)
		  jp2_image->comps[c].data[index] = *in++;
	    } else {
	      unsigned short *in = row;
	      for (unsigned int x = 0; x < img->width(); x++, index++)
		for (unsigned char c = 0; c < T_CHANNELS(cmsTempType); c++)
		  jp2_image->comps[c].data[index] = *in++;
	    }
	  }
	  std::cerr << "\r\tTransformed " << y + 1 << " of " << img->height() << " rows ("
		    << queue.num_rows() << " left)  ";
	}
	std::cerr << std::endl;
      } else {	// Other thread(s) transform the image data
	while (!queue.empty())
	  queue.writer_process_row();
      }
    }
    cmsDeleteTransform(transform);

    jp2_image->x0 = parameters.image_offset_x0;
    jp2_image->y0 = parameters.image_offset_y0;
    jp2_image->x1 = jp2_image->x0 + (img->width() - 1) * parameters.subsampling_dx + 1;
    jp2_image->y1 = jp2_image->y0 + (img->height() - 1) * parameters.subsampling_dy + 1;

    parameters.tcp_mct = jp2_image->numcomps == 3 ? 1 : 0;

    opj_cinfo_t* cinfo = opj_create_compress(CODEC_JP2);
    opj_set_event_mgr((opj_common_ptr)cinfo, &event_mgr, (void*)this);
    opj_setup_encoder(cinfo, &parameters, jp2_image);
    opj_cio_t *cio = opj_cio_open((opj_common_ptr)cinfo, NULL, 0);
    opj_encode(cinfo, cio, jp2_image, NULL);

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ofstream ofs(_filepath, std::ios_base::out);
    if (ofs.fail())
      throw FileOpenError(_filepath.native());

    int size = cio_tell(cio);
    std::cerr << "\tWriting " << size << " bytes..." << std::endl;
    ofs.write((char*)cio->buffer, size);
    ofs.close();
    opj_cio_close(cio);
    opj_destroy_compress(cinfo);
    opj_image_destroy(jp2_image);

    std::cerr << "Done." << std::endl;
  }

}
