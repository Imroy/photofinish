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
#include <iostream>
#include <queue>
#include <list>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <string.h>
#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <omp.h>
#include <lcms2.h>
#include "ImageFile.hh"
#include "Image.hh"
#include "TransformQueue.hh"
#include "Ditherer.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  JPEGfile::JPEGfile(const fs::path filepath) :
    ImageFile(filepath)
  {}

  //! Set up a "source manager" on the given JPEG decompression structure to read from an istream
  void jpeg_istream_src(j_decompress_ptr cinfo, std::istream* is);

  //! Free the data structures of the istream source manager
  void jpeg_istream_src_free(j_decompress_ptr cinfo);

  cmsHPROFILE jpeg_read_profile(jpeg_decompress_struct*, Destination::ptr dest);

  Image::ptr JPEGfile::read(Destination::ptr dest) const {
    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream fb(_filepath, std::ios_base::in);
    if (fb.fail())
      throw FileOpenError(_filepath.native());

    jpeg_decompress_struct cinfo;
    jpeg_create_decompress(&cinfo);
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);

    jpeg_istream_src(&cinfo, &fb);

    jpeg_save_markers(&cinfo, JPEG_APP0 + 2, 0xFFFF);

    jpeg_read_header(&cinfo, TRUE);
    cinfo.dct_method = JDCT_FLOAT;

    jpeg_start_decompress(&cinfo);

    Image::ptr img(new Image(cinfo.output_width, cinfo.output_height));
    dest->set_depth(8);

    if (cinfo.saw_JFIF_marker) {
      switch (cinfo.density_unit) {
      case 1:	// pixels per inch (yuck)
	img->set_resolution(cinfo.X_density, cinfo.Y_density);
	break;

      case 2:	// pixels per centimetre
	img->set_resolution(cinfo.X_density * 2.54, cinfo.Y_density * 2.54);
	break;

      default:
	std::cerr << "** Unknown density unit (" << (int)cinfo.density_unit << ") **" << std::endl;
      }
    }

    cmsUInt32Number cmsType = CHANNELS_SH(cinfo.num_components) | BYTES_SH(1);
    switch (cinfo.jpeg_color_space) {
    case JCS_GRAYSCALE:
      cmsType |= COLORSPACE_SH(PT_GRAY);
      img->set_greyscale();
      break;

    case JCS_YCbCr:
      cinfo.out_color_space = JCS_RGB;
    case JCS_RGB:
      cmsType |= COLORSPACE_SH(PT_RGB);
      break;

    case JCS_YCCK:
      cinfo.out_color_space = JCS_CMYK;
    case JCS_CMYK:
      cmsType |= COLORSPACE_SH(PT_CMYK);
      if (cinfo.saw_Adobe_marker)
	cmsType |= FLAVOR_SH(1);
      break;

    default:
      std::cerr << "** unsupported JPEG colour space " << cinfo.jpeg_color_space << " **" << std::endl;
      exit(1);
    }

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHPROFILE profile = jpeg_read_profile(&cinfo, dest);

    if (profile == NULL)
      profile = this->default_profile(cmsType);

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
	JSAMPROW jpeg_row[1];
	jpeg_row[0] = (JSAMPROW)malloc(img->width() * cinfo.output_components * sizeof(JSAMPROW));
	std::cerr << "\tReading JPEG image and transforming into L*a*b* using " << omp_get_num_threads() << " threads..." << std::endl;
	while (cinfo.output_scanline < cinfo.output_height) {
	  jpeg_read_scanlines(&cinfo, jpeg_row, 1);
	  std::cerr << "\r\tRead " << cinfo.output_scanline << " of " << img->height() << " rows ("
		    << queue.num_rows() << " in queue for colour transformation)   ";

	  queue.add_copy(cinfo.output_scanline - 1, jpeg_row[0]);

	  while (queue.num_rows() > 100) {
	    queue.reader_process_row();
	    std::cerr << "\r\tRead " << cinfo.output_scanline << " of " << img->height() << " rows ("
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

    jpeg_finish_decompress(&cinfo);
    jpeg_istream_src_free(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return img;
  }

  //! Setup a "destination manager" on the given JPEG compression structure to write to an ostream
  void jpeg_ostream_dest(j_compress_ptr cinfo, std::ostream* os);

  //! Free the data structures of the ostream destination manager
  void jpeg_ostream_dest_free(j_compress_ptr cinfo);

  void jpegfile_scan_RGB(jpeg_compress_struct* cinfo);
  void jpegfile_scan_greyscale(jpeg_compress_struct* cinfo);

  void jpeg_write_profile(jpeg_compress_struct* cinfo, unsigned char *data, unsigned int size);

  void JPEGfile::write(std::ostream& os, Image::ptr img, Destination::ptr dest) const {
    jpeg_compress_struct cinfo;
    jpeg_create_compress(&cinfo);
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);

    jpeg_ostream_dest(&cinfo, &os);

    cinfo.image_width = img->width();
    cinfo.image_height = img->height();

    cmsUInt32Number cmsTempType;
    if (img->is_greyscale()) {
      cmsTempType = COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(2);
      cinfo.input_components = 1;
      cinfo.in_color_space = JCS_GRAYSCALE;
    } else {
      cmsTempType = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(2);
      cinfo.input_components = 3;
      cinfo.in_color_space = JCS_RGB;
    }

    jpeg_set_defaults(&cinfo);
    {
      // Default values
      int quality = 95;
      int sample_h = 2, sample_v = 2;
      bool progressive = true;

      if (dest->jpeg().defined()) {
	const D_JPEG jpeg = dest->jpeg();
	if (jpeg.quality().defined())
	  quality = jpeg.quality();
	if (jpeg.progressive().defined())
	  progressive = jpeg.progressive();
	if (jpeg.sample().defined() && img->is_colour()) {
	  sample_h = jpeg.sample()->first;
	  sample_v = jpeg.sample()->second;
	}
      }

      std::cerr << "\tJPEG quality of " << quality << "." << std::endl;
      jpeg_set_quality(&cinfo, quality, TRUE);

      if (progressive) {
	std::cerr << "\tProgressive JPEG." << std::endl;
	if (img->is_greyscale())
	  jpegfile_scan_greyscale(&cinfo);
	else
	  jpegfile_scan_RGB(&cinfo);
      }

      if (img->is_colour()) {
	std::cerr << "\tJPEG chroma sub-sampling of " << sample_h << "×" << sample_v << "." << std::endl;
	cinfo.comp_info[0].h_samp_factor = sample_h;
	cinfo.comp_info[0].v_samp_factor = sample_v;
      }
    }

    cinfo.density_unit = 1;	// PPI
    cinfo.X_density = round(img->xres());
    cinfo.Y_density = round(img->yres());

    cinfo.dct_method = JDCT_FLOAT;

    jpeg_start_compress(&cinfo, TRUE);

    cmsHPROFILE profile = NULL;
    if (dest->profile()) {
      unsigned char *profile_data = NULL;
      unsigned int profile_size = 0;
      if (dest->profile()->has_data()) {
	profile_data = (unsigned char*)dest->profile()->data();
	profile_size = dest->profile()->data_size();
      } else {
	fs::ifstream ifs(dest->profile()->filepath(), std::ios_base::in);
	if (!ifs.fail()) {
	  profile_size = file_size(dest->profile()->filepath());
	  profile_data = (unsigned char*)malloc(profile_size);
	  ifs.read((char*)profile_data, profile_size);
	  ifs.close();
	}
      }

      if (profile_size > 255 * 65519)
	std::cerr << "** Profile is too big to fit in APP2 markers! **" << std::endl;
      else if (profile_size > 0) {
	profile = cmsOpenProfileFromMem(profile_data, profile_size);
	jpeg_write_profile(&cinfo, profile_data, profile_size);
      }
    }
    if (profile == NULL)
      profile = this->default_profile(cmsTempType);

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHTRANSFORM transform = cmsCreateTransform(lab, IMAGE_TYPE,
						 profile, cmsTempType,
						 INTENT_PERCEPTUAL, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    Ditherer ditherer(img->width(), cinfo.input_components);

    transform_queue queue(dest, img, cinfo.input_components, transform);
    for (unsigned int y = 0; y < cinfo.image_height; y++)
      queue.add(y);

#pragma omp parallel shared(queue)
    {
      int th_id = omp_get_thread_num();
      if (th_id == 0) {		// Master thread
	JSAMPROW jpeg_row[1];
	jpeg_row[0] = (JSAMPROW)malloc(img->width() * cinfo.input_components * sizeof(JSAMPROW));

	std::cerr << "\tTransforming from L*a*b* and writing " << img->width() << "×" << img->height() << " JPEG image using " << omp_get_num_threads() << " threads..." << std::endl;
	while (cinfo.next_scanline < cinfo.image_height) {
	  unsigned int y = cinfo.next_scanline;
	  // Process rows until the one we need becomes available, or the queue is empty
	  {
	    short unsigned int *row = queue.row(y);
	    while (!queue.empty() && (row == NULL)) {
	      queue.writer_process_row();
	      row = queue.row(y);
	    }

	    // If it's still not available, something has gone wrong
	    if (row == NULL) {
	      std::cerr << "** Oh crap (y=" << y << ", num_rows=" << queue.num_rows() << ") **" << std::endl;
	      exit(2);
	    }

	    ditherer.dither(row, jpeg_row[0], y == img->height() - 1);
	  }
	  queue.free_row(y);
	  jpeg_write_scanlines(&cinfo, jpeg_row, 1);
	  std::cerr << "\r\tTransformed and written " << y + 1 << " of " << img->height() << " rows ("
		    << queue.num_rows() << " left)   ";
	}
	std::cerr << std::endl;
	free(jpeg_row[0]);
      } else {	// Other thread(s) transform the image data
	while (!queue.empty())
	  queue.writer_process_row();
      }
    }
    cmsDeleteTransform(transform);

    jpeg_finish_compress(&cinfo);
    jpeg_ostream_dest_free(&cinfo);
    jpeg_destroy_compress(&cinfo);
  }

  void JPEGfile::write(Image::ptr img, Destination::ptr dest) const {
    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ofstream ofs(_filepath, std::ios_base::out);
    if (ofs.fail())
      throw FileOpenError(_filepath.native());

    write(ofs, img, dest);
    ofs.close();

    std::cerr << "Done." << std::endl;
  }

}
