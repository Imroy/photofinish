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
    ImageFile(filepath),
    _dinfo(NULL), _cinfo(NULL)
  {}

  //! Set up a "source manager" on the given JPEG decompression structure to read from an istream
  void jpeg_istream_src(j_decompress_ptr dinfo, std::istream* is);

  //! Free the data structures of the istream source manager
  void jpeg_istream_src_free(j_decompress_ptr dinfo);

  //! Read an ICC profile from APP2 markers in a JPEG file
  cmsHPROFILE jpeg_read_profile(jpeg_decompress_struct*, Destination::ptr dest);

  Image::ptr JPEGfile::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream ifs(_filepath, std::ios_base::in);
    if (ifs.fail())
      throw FileOpenError(_filepath.native());

    _dinfo = (jpeg_decompress_struct*)malloc(sizeof(jpeg_decompress_struct));
    jpeg_create_decompress(_dinfo);
    struct jpeg_error_mgr jerr;
    _dinfo->err = jpeg_std_error(&jerr);

    jpeg_istream_src(_dinfo, &ifs);

    jpeg_save_markers(_dinfo, JPEG_APP0 + 2, 0xFFFF);

    jpeg_read_header(_dinfo, TRUE);
    _dinfo->dct_method = JDCT_FLOAT;

    jpeg_start_decompress(_dinfo);

    Image::ptr img(new Image(_dinfo->output_width, _dinfo->output_height));
    dest->set_depth(8);

    if (_dinfo->saw_JFIF_marker) {
      switch (_dinfo->density_unit) {
      case 1:	// pixels per inch (yuck)
	img->set_resolution(_dinfo->X_density, _dinfo->Y_density);
	break;

      case 2:	// pixels per centimetre
	img->set_resolution(_dinfo->X_density * 2.54, _dinfo->Y_density * 2.54);
	break;

      default:
	std::cerr << "** Unknown density unit (" << (int)_dinfo->density_unit << ") **" << std::endl;
      }
    }

    cmsUInt32Number cmsType = CHANNELS_SH(_dinfo->num_components) | BYTES_SH(1);
    switch (_dinfo->jpeg_color_space) {
    case JCS_GRAYSCALE:
      cmsType |= COLORSPACE_SH(PT_GRAY);
      img->set_greyscale();
      break;

    case JCS_YCbCr:
      _dinfo->out_color_space = JCS_RGB;
    case JCS_RGB:
      cmsType |= COLORSPACE_SH(PT_RGB);
      break;

    case JCS_YCCK:
      _dinfo->out_color_space = JCS_CMYK;
    case JCS_CMYK:
      cmsType |= COLORSPACE_SH(PT_CMYK);
      if (_dinfo->saw_Adobe_marker)
	cmsType |= FLAVOR_SH(1);
      break;

    default:
      std::cerr << "** unsupported JPEG colour space " << _dinfo->jpeg_color_space << " **" << std::endl;
      exit(1);
    }

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHPROFILE profile = jpeg_read_profile(_dinfo, dest);

    if (profile == NULL)
      profile = this->default_profile(cmsType);

    cmsHTRANSFORM transform = cmsCreateTransform(profile, cmsType,
						 lab, IMAGE_TYPE,
						 INTENT_PERCEPTUAL, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    transform_queue queue(dest, img, cmsType, transform);

#pragma omp parallel shared(queue)
    {
      int th_id = omp_get_thread_num();
      if (th_id == 0) {		// Master thread
	JSAMPROW jpeg_row[1];
	jpeg_row[0] = (JSAMPROW)malloc(img->width() * _dinfo->output_components * sizeof(JSAMPROW));
	std::cerr << "\tReading JPEG image and transforming into L*a*b* using " << omp_get_num_threads() << " threads..." << std::endl;
	while (_dinfo->output_scanline < _dinfo->output_height) {
	  jpeg_read_scanlines(_dinfo, jpeg_row, 1);
	  std::cerr << "\r\tRead " << _dinfo->output_scanline << " of " << img->height() << " rows ("
		    << queue.num_rows() << " in queue for colour transformation)   ";

	  queue.add_copy(_dinfo->output_scanline - 1, jpeg_row[0]);

	  while (queue.num_rows() > 100) {
	    queue.reader_process_row();
	    std::cerr << "\r\tRead " << _dinfo->output_scanline << " of " << img->height() << " rows ("
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

    jpeg_finish_decompress(_dinfo);
    jpeg_istream_src_free(_dinfo);
    jpeg_destroy_decompress(_dinfo);
    free(_dinfo);
    _dinfo = NULL;
    _is_open = false;

    return img;
  }

  //! Setup a "destination manager" on the given JPEG compression structure to write to an ostream
  void jpeg_ostream_dest(j_compress_ptr cinfo, std::ostream* os);

  //! Free the data structures of the ostream destination manager
  void jpeg_ostream_dest_free(j_compress_ptr cinfo);

  //! Create a scan "script" for an RGB image
  void jpegfile_scan_RGB(jpeg_compress_struct* cinfo);

  //! Create a scan "script" for a greyscale image
  void jpegfile_scan_greyscale(jpeg_compress_struct* cinfo);

  //! Write an ICC profile into APP2 markers in a JPEG file
  void jpeg_write_profile(jpeg_compress_struct* cinfo, unsigned char *data, unsigned int size);

  void JPEGfile::mark_sGrey(cmsUInt32Number intent) const {
    if (!_is_open)
      throw FileOpenError("not open");
  }

  void JPEGfile::mark_sRGB(cmsUInt32Number intent) const {
    if (!_is_open)
      throw FileOpenError("not open");
  }

  void JPEGfile::embed_icc(std::string name, unsigned char *data, unsigned int len) const {
    if (!_is_open)
      throw FileOpenError("not open");

    if (len > 255 * 65519)
      std::cerr << "** Profile is too big to fit in APP2 markers! **" << std::endl;
    else if (len > 0)
      jpeg_write_profile(_cinfo, data, len);
  }

  void JPEGfile::write(std::ostream& os, Image::ptr img, Destination::ptr dest, bool can_free) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    _cinfo = (jpeg_compress_struct*)malloc(sizeof(jpeg_compress_struct));
    jpeg_create_compress(_cinfo);
    jpeg_error_mgr jerr;
    _cinfo->err = jpeg_std_error(&jerr);

    jpeg_ostream_dest(_cinfo, &os);

    _cinfo->image_width = img->width();
    _cinfo->image_height = img->height();

    cmsUInt32Number cmsTempType = Ditherer::cmsBaseType;
    if (img->is_greyscale()) {
      cmsTempType |= COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1);
      _cinfo->in_color_space = JCS_GRAYSCALE;
    } else {
      cmsTempType |= COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3);
      _cinfo->in_color_space = JCS_RGB;
    }
    _cinfo->input_components = T_CHANNELS(cmsTempType);

    jpeg_set_defaults(_cinfo);
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
      jpeg_set_quality(_cinfo, quality, TRUE);

      if (progressive) {
	std::cerr << "\tProgressive JPEG." << std::endl;
	if (img->is_greyscale())
	  jpegfile_scan_greyscale(_cinfo);
	else
	  jpegfile_scan_RGB(_cinfo);
      }

      if (img->is_colour()) {
	std::cerr << "\tJPEG chroma sub-sampling of " << sample_h << "×" << sample_v << "." << std::endl;
	_cinfo->comp_info[0].h_samp_factor = sample_h;
	_cinfo->comp_info[0].v_samp_factor = sample_v;
      }
    }

    _cinfo->density_unit = 1;	// PPI
    _cinfo->X_density = round(img->xres());
    _cinfo->Y_density = round(img->yres());

    _cinfo->dct_method = JDCT_FLOAT;

    jpeg_start_compress(_cinfo, TRUE);

    cmsUInt32Number intent = INTENT_PERCEPTUAL;	// Default value
    if (dest->intent().defined())
      intent = dest->intent();

    cmsHPROFILE profile = this->get_and_embed_profile(dest, cmsTempType, intent);

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHTRANSFORM transform = cmsCreateTransform(lab, IMAGE_TYPE,
						 profile, cmsTempType,
						 INTENT_PERCEPTUAL, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    Ditherer ditherer(img->width(), _cinfo->input_components);

    transform_queue queue(dest, img, cmsTempType, transform);
    for (unsigned int y = 0; y < _cinfo->image_height; y++)
      queue.add(y);

#pragma omp parallel shared(queue)
    {
      int th_id = omp_get_thread_num();
      if (th_id == 0) {		// Master thread
	JSAMPROW jpeg_row[1];
	jpeg_row[0] = (JSAMPROW)malloc(img->width() * _cinfo->input_components * sizeof(JSAMPROW));

	std::cerr << "\tTransforming from L*a*b* and writing " << img->width() << "×" << img->height() << " JPEG image using " << omp_get_num_threads() << " threads..." << std::endl;
	while (_cinfo->next_scanline < _cinfo->image_height) {
	  unsigned int y = _cinfo->next_scanline;
	  // Process rows until the one we need becomes available, or the queue is empty
	  {
	    short unsigned int *row = (short unsigned int*)queue.row(y);
	    while (!queue.empty() && (row == NULL)) {
	      queue.writer_process_row();
	      row = (short unsigned int*)queue.row(y);
	    }

	    // If it's still not available, something has gone wrong
	    if (row == NULL) {
	      std::cerr << "** Oh crap (y=" << y << ", num_rows=" << queue.num_rows() << ") **" << std::endl;
	      exit(2);
	    }

	    if (can_free)
	      img->free_row(y);
	    ditherer.dither(row, jpeg_row[0], y == img->height() - 1);
	  }
	  queue.free_row(y);
	  jpeg_write_scanlines(_cinfo, jpeg_row, 1);
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

    jpeg_finish_compress(_cinfo);
    jpeg_ostream_dest_free(_cinfo);
    jpeg_destroy_compress(_cinfo);
    free(_cinfo);
    _cinfo = NULL;
    _is_open = false;
  }

  void JPEGfile::write(Image::ptr img, Destination::ptr dest, bool can_free) {
    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ofstream ofs(_filepath, std::ios_base::out);
    if (ofs.fail())
      throw FileOpenError(_filepath.native());

    write(ofs, img, dest, can_free);
    ofs.close();

    std::cerr << "Done." << std::endl;
  }

}
