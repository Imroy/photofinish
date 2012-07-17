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
#include <queue>
#include <list>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>
#include "ImageFile.hh"
#include "Image.hh"
#include "TransformQueue.hh"
#include "Ditherer.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  PNGFile::PNGFile(const fs::path filepath) :
    ImageFile(filepath)
  {}

  //! Structure holding a PNG row to be transformed
  struct png_workqueue_row_t {
    png_uint_32 row_num;
    png_bytep row_data;

    inline png_workqueue_row_t(png_uint_32 rn, png_bytep rd) : row_num(rn), row_data(rd) {}
  };

  //! Structure holding information for the PNG progressive reader
  struct png_callback_state_t {
    bool finished;
    std::queue<png_workqueue_row_t*, std::list<png_workqueue_row_t*> > rowqueue;
    size_t rowlen;
    omp_lock_t *queue_lock;
    png_uint_32 width, height;
    Image::ptr img;
    cmsHTRANSFORM transform;
  };

  //! Called by libPNG when the iHDR chunk has been read with the main "header" information
  void png_info_cb(png_structp png, png_infop info) {
    png_set_gamma(png, 1.0, 1.0);
    png_set_alpha_mode(png, PNG_ALPHA_PNG, 1.0);
    png_set_packing(png);
    png_set_swap(png);
    png_read_update_info(png, info);

    png_callback_state_t *cs = (png_callback_state_t*)png_get_progressive_ptr(png);

    int bit_depth, colour_type;
    png_get_IHDR(png, info, &cs->width, &cs->height, &bit_depth, &colour_type, NULL, NULL, NULL);
    std::cerr << "\t" << cs->width << "×" << cs->height << ", " << bit_depth << " bpp, type " << colour_type << "." << std::endl;

    cs->img = Image::ptr(new Image(cs->width, cs->height));

    cmsUInt32Number cmsType;
    switch (colour_type) {
    case PNG_COLOR_TYPE_GRAY:
      cmsType = COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(bit_depth >> 3);
      cs->rowlen = cs->width * (bit_depth >> 3);
      cs->img->set_greyscale();
      break;
    case PNG_COLOR_TYPE_RGB:
      cmsType = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(bit_depth >> 3);
      cs->rowlen = cs->width * 3 * (bit_depth >> 3);
      break;
    default:
      std::cerr << "** unsupported PNG colour type " << colour_type << " **" << std::endl;
      exit(1);
    }

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHPROFILE profile = NULL;

    if (png_get_valid(png, info, PNG_INFO_iCCP)) {
      std::cerr << "\tImage has iCCP chunk." << std::endl;
      char *profile_name;
      int compression_type;
      png_bytep profile_data;
      unsigned int profile_len;
      if (png_get_iCCP(png, info, &profile_name, &compression_type, &profile_data, &profile_len) == PNG_INFO_iCCP) {
	std::cerr << "\tLoading ICC profile \"" << profile_name << "\" from file..." << std::endl;
	profile = cmsOpenProfileFromMem(profile_data, profile_len);
      }
    }
    if (profile == NULL) {
      if (T_COLORSPACE(cmsType) == PT_RGB) {
	std::cerr << "\tUsing default sRGB profile." << std::endl;
	profile = cmsCreate_sRGBProfile();
      } else {
	std::cerr << "\tUsing default greyscale profile." << std::endl;
	cmsToneCurve *gamma = cmsBuildGamma(NULL, 2.2);
	profile = cmsCreateGrayProfile(cmsD50_xyY(), gamma);
	cmsFreeToneCurve(gamma);
      }
    }

    cs->transform = cmsCreateTransform(profile, cmsType,
				       lab, IMAGE_TYPE,
				       INTENT_PERCEPTUAL, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);
  }

  //! Called by libPNG when a row of image data has been read
  void png_row_cb(png_structp png, png_bytep row_data, png_uint_32 row_num, int pass) {
    png_callback_state_t *cs = (png_callback_state_t*)png_get_progressive_ptr(png);
    std::cerr << "\r\tRead " << (row_num + 1) << " of " << cs->height << " rows ("
	      << cs->rowqueue.size() << " in queue for colour transformation)   ";
    png_bytep new_row = (png_bytep)malloc(cs->rowlen);
    memcpy(new_row, row_data, cs->rowlen);
    png_workqueue_row_t *row = new png_workqueue_row_t(row_num, new_row);

    omp_set_lock(cs->queue_lock);
    cs->rowqueue.push(row);
    omp_unset_lock(cs->queue_lock);
  }

  //! Called by libPNG when the image data has finished
  void png_end_cb(png_structp png, png_infop info) {
    //  png_callback_state_t *cs = (png_callback_state_t*)png_get_progressive_ptr(png);
  }

  //! Pull a row off of the workqueue and transform it using LCMS
  void png_process_row(png_callback_state_t* cs) {
    if (!cs->rowqueue.empty()) {
      omp_set_lock(cs->queue_lock);
      png_workqueue_row_t *row = cs->rowqueue.front();
      cs->rowqueue.pop();
      omp_unset_lock(cs->queue_lock);

      cmsDoTransform(cs->transform, row->row_data, cs->img->row(row->row_num), cs->width);
      free(row->row_data);
      delete row;
    }
  }

  //! Loop, processing rows from the workqueue until it's empty and reading is finished
  void png_run_workqueue(png_callback_state_t* cs) {
    while (!(cs->rowqueue.empty() && cs->finished)) {
      while (cs->rowqueue.empty() && !cs->finished)
	usleep(100);
      png_process_row(cs);
    }
  }

  Image::ptr PNGFile::read(void) const {
    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream fb(_filepath, std::ios_base::in);
    if (fb.fail())
      throw FileOpenError(_filepath.native());

    {
      unsigned char header[8];
      fb.read((char*)header, 8);
      if (png_sig_cmp(header, 0, 8))
	throw FileContentError(_filepath.string(), "is not a PNG file");
      fb.seekg(0, std::ios_base::beg);
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					     NULL, NULL, NULL);
    if (!png)
      throw LibraryError("libpng", "Could not create PNG read structure");

    png_infop info = png_create_info_struct(png);
    if (!info) {
      png_destroy_read_struct(&png, (png_infopp)NULL, (png_infopp)NULL);
      throw LibraryError("libpng", "Could not create PNG info structure");
    }

    if (setjmp(png_jmpbuf(png))) {
      png_destroy_read_struct(&png, &info, NULL);
      fb.close();
      throw LibraryError("libpng", "Something went wrong reading the PNG");
    }

    png_callback_state_t cs;
    cs.queue_lock = (omp_lock_t*)malloc(sizeof(omp_lock_t));
    omp_init_lock(cs.queue_lock);
    cs.width = cs.height = 0;
    cs.transform = NULL;
    cs.finished = false;
#pragma omp parallel shared(cs)
    {
      int th_id = omp_get_thread_num();
      if (th_id == 0) {		// Master thread
	std::cerr << "\tReading PNG image and transforming into L*a*b* using " << omp_get_num_threads() << " threads..." << std::endl;
	png_set_progressive_read_fn(png, (void *)&cs, png_info_cb, png_row_cb, png_end_cb);
	png_byte buffer[1048576];
	size_t length;
	do {
	  fb.read((char*)buffer, 1048576);
	  length = fb.gcount();
	  png_process_data(png, info, buffer, length);
	  while (cs.rowqueue.size() > 100)
	    png_process_row(&cs);
	} while (length > 0);
	std::cerr << std::endl;
	cs.finished = true;
	png_run_workqueue(&cs);	// Help finish off the transforming of image data
      } else {
	png_run_workqueue(&cs);	// Worker threads just run the workqueue
      }
    }
    omp_destroy_lock(cs.queue_lock);
    free(cs.queue_lock);
    cmsDeleteTransform(cs.transform);

    std::cerr << "Done." << std::endl;
    png_destroy_read_struct(&png, &info, NULL);
    fb.close();

    return cs.img;
  }

  //! libPNG callback for writing to an ostream
  void png_write_ostream_cb(png_structp png, png_bytep buffer, png_size_t length) {
    std::ostream *os = (std::ostream*)png_get_io_ptr(png);
    os->write((char*)buffer, length);
  }

  //! libPNG callback for flushing an ostream
  void png_flush_ostream_cb(png_structp png) {
    std::ostream *os = (std::ostream*)png_get_io_ptr(png);
    os->flush();
  }

  void PNGFile::write(Image::ptr img, const Destination &dest, const Tags &tags) const {
    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ofstream fb;
    fb.open(_filepath, std::ios_base::out);

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
					      NULL, NULL, NULL);
    if (!png)
      throw LibraryError("libpng", "Could not create PNG write structure");

    png_infop info = png_create_info_struct(png);
    if (!info) {
      png_destroy_write_struct(&png, (png_infopp)NULL);
      throw LibraryError("libpng", "Could not create PNG info structure");
    }

    if (setjmp(png_jmpbuf(png))) {
      png_destroy_write_struct(&png, &info);
      fb.close();
      throw LibraryError("libpng", "Something went wrong writing the PNG");
    }

    png_set_write_fn(png, &fb, png_write_ostream_cb, png_flush_ostream_cb);

    int png_colour_type, png_channels;
    cmsUInt32Number cmsTempType;
    if (img->is_colour()) {
      png_colour_type = PNG_COLOR_TYPE_RGB;
      png_channels = 3;
      cmsTempType = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(2);
    } else {
      png_colour_type = PNG_COLOR_TYPE_GRAY;
      png_channels = 1;
      cmsTempType = COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(2);
    }

    int depth = 8;	// Default value
    if (dest.depth().defined())
      depth = dest.depth();

    std::cerr << "\tWriting header for " << img->width() << "×" << img->height()
	      << " " << depth << "-bit " << (png_channels == 1 ? "greyscale" : "RGB")
	      << " PNG image..." << std::endl;
    png_set_IHDR(png, info,
		 img->width(), img->height(), depth, png_colour_type,
		 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_filter(png, 0, PNG_ALL_FILTERS);
    png_set_compression_level(png, Z_BEST_COMPRESSION);

    cmsUInt32Number intent = INTENT_PERCEPTUAL;	// Default value
    if (dest.intent().defined())
      intent = dest.intent();

    cmsHPROFILE profile = NULL;
    if (dest.profile().defined() && dest.profile().filepath().defined())
      profile = cmsOpenProfileFromFile(dest.profile().filepath()->c_str(), "r");

    if ((profile == NULL) && (img->is_greyscale())) {
      std::cerr << "\tUsing default greyscale profile." << std::endl;
      cmsToneCurve *gamma = cmsBuildGamma(NULL, 2.2);
      profile = cmsCreateGrayProfile(cmsD50_xyY(), gamma);
      cmsFreeToneCurve(gamma);
    }

    if (profile != NULL) {
      cmsUInt32Number len;
      cmsSaveProfileToMem(profile, NULL, &len);
      if (len > 0) {
	png_bytep profile_data = (png_bytep)malloc(len);
	if (cmsSaveProfileToMem(profile, profile_data, &len)) {
	  std::string profile_name = "icc";	// Default value
	  if (dest.profile().defined() && dest.profile().name().defined())
	    profile_name = dest.profile().name();
	  std::cerr << "\tEmbedding profile \"" << profile_name << "\" (" << len << " bytes)." << std::endl;
	  png_set_iCCP(png, info, profile_name.c_str(), 0, profile_data, len);
	}
      }
    } else {
      std::cerr << "\tUsing default sRGB profile." << std::endl;
      profile = cmsCreate_sRGBProfile();
      png_set_sRGB_gAMA_and_cHRM(png, info, intent);
    }

    {
      time_t t = time(NULL);
      if (t > 0) {
	png_time ptime;
	png_convert_from_time_t(&ptime, t);
	std::cerr << "\tAdding time chunk." << std::endl;
	png_set_tIME(png, info, &ptime);
      }
    }

    png_bytepp png_rows = (png_bytepp)malloc(img->height() * sizeof(png_bytep));
    for (unsigned int y = 0; y < img->height(); y++)
      png_rows[y] = (png_bytep)malloc(img->width() * png_channels * (depth >> 3));

    png_set_rows(png, info, png_rows);

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHTRANSFORM transform = cmsCreateTransform(lab, IMAGE_TYPE,
						 profile, cmsTempType,
						 intent, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    transform_queue queue(img, png_channels, transform, depth == 8 ? NULL : png_rows);

#pragma omp parallel shared(queue)
    {
      int th_id = omp_get_thread_num();
      if (th_id == 0) {		// Master thread
	std::cerr << "\tTransforming image data from L*a*b* using " << omp_get_num_threads() << " threads." << std::endl;

	Ditherer ditherer(img->width(), png_channels);

	for (unsigned int y = 0; y < img->height(); y++) {
	  // Process rows until the one we need becomes available, or the queue is empty
	  short unsigned int *row = queue.row(y);
	  while (!queue.empty() && (row == NULL)) {
	    queue.process_row();
	    row = queue.row(y);
	  }

	  // If it's still not available, something has gone wrong
	  if (row == NULL) {
	    std::cerr << "** Oh crap (y=" << y << ", num_rows=" << queue.num_rows() << " **" << std::endl;
	    exit(2);
	  }

	  if (depth == 8) {
	    ditherer.dither(row, png_rows[y], y == img->height() - 1);
	    free(row);
	  }
	  std::cerr << "\r\tTransformed " << y + 1 << " of " << img->height() << " rows ("
		    << queue.num_rows() << " left)  ";
	}
      } else {	// Other thread(s) transform the image data
	while (!queue.empty())
	  queue.process_row();
      }
    }
    cmsDeleteTransform(transform);
    std::cerr << std::endl;

    std::cerr << "\tWriting PNG image data..." << std::endl;
    png_write_png(png, info, PNG_TRANSFORM_SWAP_ENDIAN, NULL);

    for (unsigned int y = 0; y < img->height(); y++)
      free(png_rows[y]);
    free(png_rows);

    png_destroy_write_struct(&png, &info);

    std::cerr << "Done." << std::endl;
    fb.close();

    tags.embed(_filepath);
  }

}
