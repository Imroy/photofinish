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

  template <typename T>
  void* planar_to_packed(unsigned int width, unsigned char channels, opj_image_t* image, unsigned int& index) {
    T *row = (T*)malloc(width * channels * sizeof(T));
    T *out = row;
    for (unsigned int x = 0; x < width; x++, index++)
      for (unsigned char c = 0; c < channels; c++)
	*out++ = image->comps[c].data[index];
    return (void*)row;
  }

  template <typename T>
  void packed_to_planar(unsigned int width, unsigned char channels, T* row, opj_image_t* image, unsigned int& index) {
    T *in = row;
    for (unsigned int x = 0; x < width; x++, index++)
      for (unsigned char c = 0; c < channels; c++)
	image->comps[c].data[index] = *in++;
  }

  Image::ptr JP2file::read(Destination::ptr dest) const {
    opj_event_mgr_t event_mgr;
    memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
    event_mgr.error_handler = error_callback;
    event_mgr.warning_handler = warning_callback;
    event_mgr.info_handler = info_callback;

    opj_dparameters_t parameters;
    opj_set_default_decoder_parameters(&parameters);

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream ifs(_filepath, std::ios_base::in);
    if (ifs.fail())
      throw FileOpenError(_filepath.native());

    unsigned int file_size = fs::file_size(_filepath);
    unsigned char *src = (unsigned char *)malloc(file_size);
    ifs.read((char*)src, file_size);

    opj_dinfo_t *dinfo = opj_create_decompress(CODEC_JP2);
    opj_set_event_mgr((opj_common_ptr)dinfo, &event_mgr, (void*)this);
    opj_setup_decoder(dinfo, &parameters);
    opj_cio_t *cio = opj_cio_open((opj_common_ptr)dinfo, src, file_size);
    opj_image_t *jp2_image = opj_decode(dinfo, cio);
    if (jp2_image == NULL)
      throw LibraryError("OpenJPEG", "Could not decode file");

    opj_cio_close(cio);
    free(src);

    // Is this necessary?
    if (jp2_image->numcomps > 1)
      for (unsigned char c = 1; c < jp2_image->numcomps; c++) {
	if (jp2_image->comps[c].dx != jp2_image->comps[0].dx)
	  std::cerr << "** Component " << (int)c << " has a different dx to the first! **" << std::endl;
	if (jp2_image->comps[c].dy != jp2_image->comps[0].dy)
	  std::cerr << "** Component " << (int)c << " has a different dy to the first! **" << std::endl;
	if (jp2_image->comps[c].w != jp2_image->comps[0].w)
	  std::cerr << "** Component " << (int)c << " has a different w to the first! **" << std::endl;
	if (jp2_image->comps[c].h != jp2_image->comps[0].h)
	  std::cerr << "** Component " << (int)c << " has a different h to the first! **" << std::endl;
	if (jp2_image->comps[c].x0 != jp2_image->comps[0].x0)
	  std::cerr << "** Component " << (int)c << " has a different x0 to the first! **" << std::endl;
	if (jp2_image->comps[c].y0 != jp2_image->comps[0].y0)
	  std::cerr << "** Component " << (int)c << " has a different y0 to the first! **" << std::endl;
	if (jp2_image->comps[c].prec != jp2_image->comps[0].prec)
	  std::cerr << "** Component " << (int)c << " has a different prec to the first! **" << std::endl;
	if (jp2_image->comps[c].bpp != jp2_image->comps[0].bpp)
	  std::cerr << "** Component " << (int)c << " has a different bpp to the first! **" << std::endl;
	if (jp2_image->comps[c].sgnd != jp2_image->comps[0].sgnd)
	  std::cerr << "** Component " << (int)c << " has a different sgnd to the first! **" << std::endl;
      }

    Image::ptr img(new Image(jp2_image->x1 - jp2_image->x0, jp2_image->y1 - jp2_image->y0));

    int depth = jp2_image->comps[0].prec;
    cmsUInt32Number cmsType = CHANNELS_SH(jp2_image->numcomps) | BYTES_SH(depth >> 3);
    switch (jp2_image->color_space) {
    case CLRSPC_SRGB:
      cmsType |= COLORSPACE_SH(PT_RGB);
      break;

    case CLRSPC_GRAY:
      img->set_greyscale();
      cmsType |= COLORSPACE_SH(PT_GRAY);
      break;

    case CLRSPC_SYCC:
      cmsType |= COLORSPACE_SH(PT_YUV);
      break;

    default:
      std::cerr << "** Unknown colour space " << jp2_image->color_space << " **" << std::endl;
      throw LibraryError("OpenJPEG", "color_space");
    }

    cmsHPROFILE profile;
    if (jp2_image->icc_profile_buf != NULL)
      profile = cmsOpenProfileFromMem(jp2_image->icc_profile_buf, jp2_image->icc_profile_len);
    else
      profile = ImageFile::default_profile(cmsType);

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHTRANSFORM transform = cmsCreateTransform(profile, cmsType,
						 lab, IMAGE_TYPE,
						 INTENT_PERCEPTUAL, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    transform_queue queue(dest, img, T_CHANNELS(cmsType), transform);

#pragma omp parallel shared(queue)
    {
      int th_id = omp_get_thread_num();
      if (th_id == 0) {		// Master thread
	std::cerr << "\tTransforming into L*a*b* using " << omp_get_num_threads() << " threads..." << std::endl;
	unsigned int index = 0;
	for (unsigned int y = 0; y < img->height(); y++) {
	  void *buffer = NULL;
	  if (depth == 8)
	    buffer = planar_to_packed<unsigned char>(img->width(), T_CHANNELS(cmsType), jp2_image, index);
	  else
	    buffer = planar_to_packed<unsigned short>(img->width(), T_CHANNELS(cmsType), jp2_image, index);

	  std::cerr << "\r\tRead " << (y + 1) << " of " << img->height() << " rows ("
		    << queue.num_rows() << " in queue for colour transformation)   ";

	  queue.add_copy(y, buffer);

	  while (queue.num_rows() > 100) {
	    queue.reader_process_row();
	    std::cerr << "\r\tRead " << (y + 1) << " of " << img->height() << " rows ("
		      << queue.num_rows() << " in queue for colour transformation)   ";
	  }
	}
	queue.set_finished();
	while (!queue.empty()) {
	  queue.reader_process_row();
	  std::cerr << "\r\tRead " << img->height() << " of " << img->height() << " rows ("
		    << queue.num_rows() << " in queue for colour transformation)   ";
	}
	std::cerr << std::endl;
      } else {
	while (!(queue.empty() && queue.finished())) {
	  while (queue.empty() && !queue.finished())
	    usleep(100);
	  queue.reader_process_row();
	}
      }
    }
    cmsDeleteTransform(transform);

    std::cerr << "Done." << std::endl;
    return img;
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

	unsigned int index = 0;
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

	    if (depth == 8) {
	      ditherer.dither(row, rows[y], y == img->height() - 1);
	      queue.free_row(y);

	      packed_to_planar<unsigned char>(img->width(), T_CHANNELS(cmsTempType), rows[y], jp2_image, index);
	    } else
	      packed_to_planar<unsigned short>(img->width(), T_CHANNELS(cmsTempType), row, jp2_image, index);
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
