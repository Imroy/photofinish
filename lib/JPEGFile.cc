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
#include "Ditherer.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  JPEGFile::JPEGFile(const fs::path filepath) :
    ImageFile(filepath)
  {}

  Image::ptr JPEGFile::read(void) const {
    throw Unimplemented("JPEGFile", "read()");
  }

  //! Structure holding information for the ostream writer
  struct jpeg_destination_state_t {
    JOCTET *buffer;
    std::ostream *os;
    size_t buffer_size;
  };

  //! Called by libJPEG to initialise the "destination manager"
  static void jpeg_ostream_init_destination(j_compress_ptr cinfo) {
    jpeg_destination_mgr *dmgr = (jpeg_destination_mgr*)(cinfo->dest);
    jpeg_destination_state_t *ds = (jpeg_destination_state_t*)(cinfo->client_data);
    ds->buffer = (JOCTET*)malloc(ds->buffer_size);
    if (ds->buffer == NULL)
      throw MemAllocError("Out of memory?");

    dmgr->next_output_byte = ds->buffer;
    dmgr->free_in_buffer = ds->buffer_size;
  }

  //! Called by libJPEG to write the output buffer and prepare it for more data
  static boolean jpeg_ostream_empty_output_buffer(j_compress_ptr cinfo) {
    jpeg_destination_mgr *dmgr = (jpeg_destination_mgr*)(cinfo->dest);
    jpeg_destination_state_t *ds = (jpeg_destination_state_t*)(cinfo->client_data);
    ds->os->write((char*)ds->buffer, ds->buffer_size);
    dmgr->next_output_byte = ds->buffer;
    dmgr->free_in_buffer = ds->buffer_size;
    return 1;
  }

  //! Called by libJPEG to write any remaining data in the output buffer and deallocate it
  static void jpeg_ostream_term_destination(j_compress_ptr cinfo) {
    jpeg_destination_mgr *dmgr = (jpeg_destination_mgr*)(cinfo->dest);
    jpeg_destination_state_t *ds = (jpeg_destination_state_t*)(cinfo->client_data);
    ds->os->write((char*)ds->buffer, ds->buffer_size - dmgr->free_in_buffer);
    free(ds->buffer);
    ds->buffer = NULL;
    dmgr->free_in_buffer = 0;
  }

  void jpegfile_scan_RGB(jpeg_compress_struct* cinfo);
  void jpegfile_scan_greyscale(jpeg_compress_struct* cinfo);

  //! Class holding information for the JPEG writer
  class jpeg_write_queue {
  private:
    std::queue<long int, std::list<long int> > rownumbers;
    short unsigned int **rows;
    std::vector<omp_lock_t*> rowlocks;
    size_t rowlen;
    omp_lock_t *queue_lock;
    Image::ptr img;
    cmsHTRANSFORM transform;

  public:
    jpeg_write_queue(Image::ptr _img, int channels, cmsHTRANSFORM t) :
      rows((short unsigned int**)malloc(_img->height() * sizeof(short unsigned int*))),
      rowlocks(_img->height()),
      rowlen(_img->width() * channels * sizeof(short unsigned int)),
      queue_lock((omp_lock_t*)malloc(sizeof(omp_lock_t))),
      img(_img),
      transform(t)
    {
      omp_init_lock(queue_lock);

      for (long int y = 0; y < img->height(); y++) {
	rownumbers.push(y);
	rows[y] = NULL;
	rowlocks[y] = (omp_lock_t*)malloc(sizeof(omp_lock_t));
	omp_init_lock(rowlocks[y]);
      }
    }

    ~jpeg_write_queue() {
      for (long int y = 0; y < img->height(); y++) {
	omp_destroy_lock(rowlocks[y]);
	free(rowlocks[y]);
      }
      free(rows);
      omp_destroy_lock(queue_lock);
      free(queue_lock);
    }

    inline size_t num_rows(void) const { return rownumbers.size(); }

    inline bool empty(void) const {
      omp_set_lock(queue_lock);
      bool ret = rownumbers.empty();
      omp_unset_lock(queue_lock);
      return ret;
    }

    inline short unsigned int* row(long int y) const {
      omp_set_lock(rowlocks[y]);
      short unsigned int *ret = rows[y];
      omp_unset_lock(rowlocks[y]);
      return ret;
    }

    //! Pull a row off of the workqueue and transform it using LCMS
    inline void process_row(void) {
      omp_set_lock(queue_lock);
      if (!rownumbers.empty()) {
	long int y = rownumbers.front();
	rownumbers.pop();

	omp_set_lock(rowlocks[y]);	// Take out rowlock before relinquishing the queue lock
	omp_unset_lock(queue_lock);

	short unsigned int *row = (short unsigned int*)malloc(rowlen);
	cmsDoTransform(transform, img->row(y), row, img->width());
	rows[y] = row;
	omp_unset_lock(rowlocks[y]);
      } else
	omp_unset_lock(queue_lock);
    }
  };

  void JPEGFile::write(std::ostream& os, Image::ptr img, const Destination &dest) const {
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    jpeg_destination_state_t ds;
    ds.os = &os;
    ds.buffer_size = 1048576;
    cinfo.client_data = (void*)&ds;

    jpeg_destination_mgr dmgr;
    dmgr.init_destination = jpeg_ostream_init_destination;
    dmgr.empty_output_buffer = jpeg_ostream_empty_output_buffer;
    dmgr.term_destination = jpeg_ostream_term_destination;
    cinfo.dest = &dmgr;

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

      if (dest.jpeg().defined()) {
	const D_JPEG jpeg = dest.jpeg();
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

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHTRANSFORM transform = cmsCreateTransform(lab, IMAGE_TYPE,
						 profile, cmsTempType,
						 INTENT_PERCEPTUAL, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    Ditherer ditherer(img->width(), cinfo.input_components);

    jpeg_write_queue queue(img, cinfo.input_components, transform);

#pragma omp parallel shared(queue)
    {
      int th_id = omp_get_thread_num();
      if (th_id == 0) {		// Master thread
	JSAMPROW jpeg_row[1];
	jpeg_row[0] = (JSAMPROW)malloc(img->height() * cinfo.input_components * sizeof(JSAMPROW));

	std::cerr << "\tTransforming from L*a*b* and writing " << img->width() << "×" << img->height() << " JPEG image using " << omp_get_num_threads() << " threads..." << std::endl;
	jpeg_start_compress(&cinfo, TRUE);
	while (cinfo.next_scanline < cinfo.image_height) {
	  int y = cinfo.next_scanline;
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

	  ditherer.dither(row, jpeg_row[0], y == img->height() - 1);
	  jpeg_write_scanlines(&cinfo, jpeg_row, 1);
	  std::cerr << "\r\tWritten " << y + 1 << " of " << img->height() << " rows ("
		    << queue.num_rows() << " left for colour transformation)   ";
	}
	free(jpeg_row[0]);
      } else {	// Other thread(s) transform the image data
	while (!queue.empty())
	  queue.process_row();
      }
    }
    cmsDeleteTransform(transform);
    std::cerr << std::endl;

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
  }

  void JPEGFile::write(Image::ptr img, const Destination &dest, const Tags &tags) const {
    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ofstream ofs(_filepath, std::ios_base::out);
    if (ofs.fail())
      throw FileOpenError(_filepath.native());

    write(ofs, img, dest);
    ofs.close();

    std::cerr << "\tEmbedding metadata..." << std::endl;
    tags.embed(_filepath);
    std::cerr << "Done." << std::endl;
  }

}
