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
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>
#include "ImageFile.hh"
#include "Image.hh"
#include "TransformQueue.hh"
#include "Ditherer.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  PNGfile::PNGfile(const fs::path filepath) :
    ImageFile(filepath),
    _png(NULL), _info(NULL)
  {}

  //! Called by libPNG when the iHDR chunk has been read with the main "header" information
  void png_info_cb(png_structp png, png_infop info) {
    png_set_gamma(png, 1.0, 1.0);
    png_set_alpha_mode(png, PNG_ALPHA_PNG, 1.0);
    png_set_packing(png);
    png_set_swap(png);
    png_read_update_info(png, info);

    transform_queue *queue = (transform_queue*)png_get_progressive_ptr(png);

    unsigned int width, height;
    int bit_depth, colour_type;
    png_get_IHDR(png, info, &width, &height, &bit_depth, &colour_type, NULL, NULL, NULL);
    std::cerr << "\t" << width << "×" << height << ", " << bit_depth << " bpp, type " << colour_type << "." << std::endl;

    Image::ptr img(new Image(width, height));
    queue->destination()->set_depth(bit_depth);

    cmsUInt32Number cmsType;
    switch (colour_type) {
    case PNG_COLOR_TYPE_GRAY:
      cmsType = COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(bit_depth >> 3);
      img->set_greyscale();
      break;
    case PNG_COLOR_TYPE_RGB:
      cmsType = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(bit_depth >> 3);
      break;
    default:
      std::cerr << "** unsupported PNG colour type " << colour_type << " **" << std::endl;
      exit(1);
    }
    queue->set_image(img, T_CHANNELS(cmsType));

    {
      unsigned int xres, yres;
      int unit_type;
      if (png_get_pHYs(png, info, &xres, &yres, &unit_type)) {
	switch (unit_type) {
	case PNG_RESOLUTION_METER:
	  img->set_resolution(xres * 0.0254, yres * 0.0254);
	  break;
	case PNG_RESOLUTION_UNKNOWN:
	  break;
	default:
	  std::cerr << "** unknown unit type " << unit_type << " **" << std::endl;
	}
	if (img->xres().defined() && img->yres().defined())
	  std::cerr << "\tImage has resolution of " << img->xres() << "×" << img->yres() << " PPI." << std::endl;
      }
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
	void *data_copy = malloc(profile_len);
	memcpy(data_copy, profile_data, profile_len);
	queue->destination()->set_profile(profile_name, data_copy, profile_len);
      }
    }
    if (profile == NULL)
      profile = ImageFile::default_profile(cmsType);

    cmsHTRANSFORM transform = cmsCreateTransform(profile, cmsType,
						 lab, IMAGE_TYPE,
						 INTENT_PERCEPTUAL, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    queue->set_transform(transform);
  }

  //! Called by libPNG when a row of image data has been read
  void png_row_cb(png_structp png, png_bytep row_data, png_uint_32 row_num, int pass) {
    transform_queue *queue = (transform_queue*)png_get_progressive_ptr(png);
    std::cerr << "\r\tRead " << (row_num + 1) << " of " << queue->image()->height() << " rows ("
	      << queue->num_rows() << " in queue for colour transformation)   ";
    queue->add_copy(row_num, row_data);
  }

  //! Called by libPNG when the image data has finished
  void png_end_cb(png_structp png, png_infop info) {
    //    transform_queue *queue = (transform_queue*)png_get_progressive_ptr(png);
  }

  //! Loop, processing rows from the workqueue until it's empty and reading is finished
  void png_run_workqueue(transform_queue *queue) {
    while (!(queue->empty() && queue->finished())) {
      while (queue->empty() && !queue->finished())
	usleep(100);
      queue->reader_process_row();
    }
  }

  Image::ptr PNGfile::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

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

    _png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					     NULL, NULL, NULL);
    if (!_png)
      throw LibraryError("libpng", "Could not create PNG read structure");

    _info = png_create_info_struct(_png);
    if (!_info) {
      png_destroy_read_struct(&_png, (png_infopp)NULL, (png_infopp)NULL);
      throw LibraryError("libpng", "Could not create PNG info structure");
    }

    if (setjmp(png_jmpbuf(_png))) {
      png_destroy_read_struct(&_png, &_info, NULL);
      fb.close();
      throw LibraryError("libpng", "Something went wrong reading the PNG");
    }

    transform_queue queue(dest);

#pragma omp parallel shared(queue)
    {
      int th_id = omp_get_thread_num();
      if (th_id == 0) {		// Master thread
	std::cerr << "\tReading PNG image and transforming into L*a*b* using " << omp_get_num_threads() << " threads..." << std::endl;
	png_set_progressive_read_fn(_png, (void *)&queue, png_info_cb, png_row_cb, png_end_cb);
	png_byte buffer[1048576];
	size_t length;
	do {
	  fb.read((char*)buffer, 1048576);
	  length = fb.gcount();
	  png_process_data(_png, _info, buffer, length);
	  while (queue.num_rows() > 100)
	    queue.reader_process_row();
	} while (length > 0);
	std::cerr << std::endl;
	queue.set_finished();
	png_run_workqueue(&queue);	// Help finish off the transforming of image data
      } else {
	png_run_workqueue(&queue);	// Worker threads just run the workqueue
      }
    }
    queue.free_transform();

    png_destroy_read_struct(&_png, &_info, NULL);
    fb.close();
    _is_open = false;

    std::cerr << "Done." << std::endl;
    return queue.image();
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

  void PNGfile::mark_sGrey(cmsUInt32Number intent) const {
    if (!_is_open)
      throw FileOpenError("not open");

    png_set_sRGB_gAMA_and_cHRM(_png, _info, intent);
  }

  void PNGfile::mark_sRGB(cmsUInt32Number intent) const {
    if (!_is_open)
      throw FileOpenError("not open");

    png_set_sRGB_gAMA_and_cHRM(_png, _info, intent);
  }

  void PNGfile::embed_icc(std::string name, unsigned char *data, unsigned int len) const {
    if (!_is_open)
      throw FileOpenError("not open");

    std::cerr << "\tEmbedding profile \"" << name << "\" (" << len << " bytes)." << std::endl;
    png_set_iCCP(_png, _info, name.c_str(), 0, data, len);
  }

  void PNGfile::write(Image::ptr img, Destination::ptr dest, bool can_free) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ofstream fb;
    fb.open(_filepath, std::ios_base::out);

    _png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
					      NULL, NULL, NULL);
    if (!_png)
      throw LibraryError("libpng", "Could not create PNG write structure");

    _info = png_create_info_struct(_png);
    if (!_info) {
      png_destroy_write_struct(&_png, (png_infopp)NULL);
      throw LibraryError("libpng", "Could not create PNG info structure");
    }

    if (setjmp(png_jmpbuf(_png))) {
      png_destroy_write_struct(&_png, &_info);
      fb.close();
      throw LibraryError("libpng", "Something went wrong writing the PNG");
    }

    png_set_write_fn(_png, &fb, png_write_ostream_cb, png_flush_ostream_cb);

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
    if (dest->depth().defined())
      depth = dest->depth();

    std::cerr << "\tWriting header for " << img->width() << "×" << img->height()
	      << " " << depth << "-bit " << (png_channels == 1 ? "greyscale" : "RGB")
	      << " PNG image..." << std::endl;
    png_set_IHDR(_png, _info,
		 img->width(), img->height(), depth, png_colour_type,
		 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_filter(_png, 0, PNG_ALL_FILTERS);
    png_set_compression_level(_png, Z_BEST_COMPRESSION);

    if (img->xres().defined() && img->yres().defined()) {
      unsigned int xres = round(img->xres() / 0.0254);
      unsigned int yres = round(img->yres() / 0.0254);
      png_set_pHYs(_png, _info, xres, yres, PNG_RESOLUTION_METER);
    }

    cmsUInt32Number intent = INTENT_PERCEPTUAL;	// Default value
    if (dest->intent().defined())
      intent = dest->intent();

    cmsHPROFILE profile = this->get_and_embed_profile(dest, cmsTempType, intent);

    {
      time_t t = time(NULL);
      if (t > 0) {
	png_time ptime;
	png_convert_from_time_t(&ptime, t);
	std::cerr << "\tAdding time chunk." << std::endl;
	png_set_tIME(_png, _info, &ptime);
      }
    }

    png_bytepp png_rows = (png_bytepp)malloc(img->height() * sizeof(png_bytep));
    for (unsigned int y = 0; y < img->height(); y++)
      png_rows[y] = (png_bytep)malloc(img->width() * png_channels * (depth >> 3));

    png_set_rows(_png, _info, png_rows);

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHTRANSFORM transform = cmsCreateTransform(lab, IMAGE_TYPE,
						 profile, cmsTempType,
						 intent, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    transform_queue queue(dest, img, png_channels, transform);
    if (depth == 8)
      for (unsigned int y = 0; y < img->height(); y++)
	queue.add(y);
    else
      for (unsigned int y = 0; y < img->height(); y++)
	queue.add(y, png_rows[y]);

#pragma omp parallel shared(queue)
    {
      int th_id = omp_get_thread_num();
      if (th_id == 0) {		// Master thread
	std::cerr << "\tTransforming image data from L*a*b* using " << omp_get_num_threads() << " threads." << std::endl;

	Ditherer ditherer(img->width(), png_channels);

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

	    if (can_free)
	      img->free_row(y);
	    if (depth == 8) {
	      ditherer.dither(row, png_rows[y], y == img->height() - 1);
	      queue.free_row(y);
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

    std::cerr << "\tWriting PNG image data..." << std::endl;
    png_write_png(_png, _info, PNG_TRANSFORM_SWAP_ENDIAN, NULL);

    for (unsigned int y = 0; y < img->height(); y++)
      free(png_rows[y]);
    free(png_rows);

    png_destroy_write_struct(&_png, &_info);
    fb.close();
    _is_open = false;

    std::cerr << "Done." << std::endl;
  }

}
