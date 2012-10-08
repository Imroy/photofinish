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

namespace fs = boost::filesystem;

namespace PhotoFinish {

  PNGreader::PNGreader(std::istream* is) :
    ImageReader(is),
    _png(NULL), _info(NULL)
  {
    {
      unsigned char header[8];
      ifs.read((char*)header, 8);
      if (png_sig_cmp(header, 0, 8))
	throw FileContentError(_filepath.string(), "is not a PNG file");
      ifs.seekg(0, std::ios_base::beg);
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
      ifs.close();
      throw LibraryError("libpng", "Something went wrong reading the PNG");
    }
  }

  void JPEGreader::do_work(void) {
    switch (_read_state) {
    case 0:
    }
  }

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
    queue->set_image(img, cmsType);

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

  Image::ptr PNGreader::read(Destination::ptr dest) {

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
	  ifs.read((char*)buffer, 1048576);
	  length = ifs.gcount();
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
    ifs.close();
    _is_open = false;

    std::cerr << "Done." << std::endl;
    return queue.image();
  }

}
