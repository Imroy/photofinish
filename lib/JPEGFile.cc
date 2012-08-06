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
#include <queue>
#include <list>
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
#include "TransformQueue.hh"
#include "Ditherer.hh"
#include "JPEGfile_iostream.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  JPEGFile::JPEGFile(const fs::path filepath) :
    ImageFile(filepath)
  {}

  Image::ptr JPEGFile::read(Destination::ptr dest) const {
    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream fb(_filepath, std::ios_base::in);
    if (fb.fail())
      throw FileOpenError(_filepath.native());

    struct jpeg_decompress_struct cinfo;
    jpeg_create_decompress(&cinfo);
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);

    jpeg_istream_src(&cinfo, &fb);

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
	std::cerr << "** Unknown density unit (" << cinfo.density_unit << ") **" << std::endl;
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

      /*
    case JCS_YCCK:
      cinfo.out_color_space = JCS_CMYK;
    case JCS_CMYK:
      cmsType |= COLORSPACE_SH(PT_CMYK);
      if (cinfo.saw_Adobe_marker)
	cmsType |= FLAVOR_SH(1);
      break;
      */

    default:
      std::cerr << "** unsupported JPEG colour space " << cinfo.jpeg_color_space << " **" << std::endl;
      exit(1);
    }

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHPROFILE profile = NULL;

    if (T_COLORSPACE(cmsType) == PT_RGB) {
      std::cerr << "\tUsing default sRGB profile." << std::endl;
      profile = cmsCreate_sRGBProfile();
    } else {
      std::cerr << "\tUsing default greyscale profile." << std::endl;
      cmsToneCurve *gamma = cmsBuildGamma(NULL, 2.2);
      profile = cmsCreateGrayProfile(cmsD50_xyY(), gamma);
      cmsFreeToneCurve(gamma);
    }

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

  void jpegfile_scan_RGB(jpeg_compress_struct* cinfo);
  void jpegfile_scan_greyscale(jpeg_compress_struct* cinfo);

  void JPEGFile::write(std::ostream& os, Image::ptr img, Destination::ptr dest) const {
    jpeg_compress_struct cinfo;
    jpeg_create_compress(&cinfo);
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);

    jpeg_ostream_dest(&cinfo, &os);

    cinfo.image_width = img->width();
    cinfo.image_height = img->height();

    cmsHPROFILE profile = NULL;
    cmsUInt32Number cmsTempType;
    if (img->is_greyscale()) {
      std::cerr << "\tUsing default greyscale profile." << std::endl;
      cmsToneCurve *gamma = cmsBuildGamma(NULL, 2.2);
      profile = cmsCreateGrayProfile(cmsD50_xyY(), gamma);
      cmsFreeToneCurve(gamma);
      cmsTempType = COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(2);
      cinfo.input_components = 1;
      cinfo.in_color_space = JCS_GRAYSCALE;
    } else {
      std::cerr << "\tUsing default sRGB profile." << std::endl;
      profile = cmsCreate_sRGBProfile();
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
	jpeg_start_compress(&cinfo, TRUE);
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

  void JPEGFile::write(Image::ptr img, Destination::ptr dest) const {
    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ofstream ofs(_filepath, std::ios_base::out);
    if (ofs.fail())
      throw FileOpenError(_filepath.native());

    write(ofs, img, dest);
    ofs.close();

    std::cerr << "Done." << std::endl;
  }

}
