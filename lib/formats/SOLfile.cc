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
#include <lcms2.h>
#include "ImageFile.hh"
#include "Image.hh"
#include "TransformQueue.hh"
#include "Ditherer.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {
  SOLfile::SOLfile(const fs::path filepath) :
    ImageFile(filepath)
  {}

  Image::ptr SOLfile::read(Destination::ptr dest) {
    throw Unimplemented("SOLfile", "read");
  }

  unsigned char header[12] = { 0x53, 0x4f, 0x4c, 0x3a, 0x00, 0x00, 0x00, 0x00,
			       0x00, 0x00, 0x00, 0x00 };

  void write_be(void *ptr, size_t size, std::ostream &stream) {
    unsigned char *in = (unsigned char*)ptr + size - 1;
    size_t n;
    for (n = 0; n < size; n++, in--)
      stream.put(*in);
  }

  void SOLfile::write(Image::ptr img, Destination::ptr dest, bool can_free) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ofstream ofs;
    ofs.open(_filepath, std::ios_base::out);

    ofs.write((char*)header, 12);
    {
      unsigned int width = img->width(), height = img->height();
      write_be(&width, 4, ofs);
      write_be(&height, 4, ofs);
    }

    cmsUInt32Number cmsType = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(2);
    cmsUInt32Number intent = INTENT_PERCEPTUAL;	// Default value
    if (dest->intent().defined())
      intent = dest->intent();

    cmsHPROFILE profile = this->default_profile(cmsType);

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHTRANSFORM transform = cmsCreateTransform(lab, IMAGE_TYPE,
						 profile, cmsType,
						 intent, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    transform_queue queue(dest, img, cmsType, transform);
    for (unsigned int y = 0; y < img->height(); y++)
      queue.add(y);

#pragma omp parallel shared(queue)
    {
      int th_id = omp_get_thread_num();
      if (th_id == 0) {		// Master thread
	std::cerr << "\tTransforming image data from L*a*b* using " << omp_get_num_threads() << " threads." << std::endl;

	Ditherer ditherer(img->width(), 3, { 31, 63, 31 });
	unsigned char *temprow = (unsigned char*)malloc(img->width() * 3);

	for (unsigned int y = 0; y < img->height(); y++) {
	  // Process rows until the one we need becomes available, or the queue is empty
	  short unsigned int *row = (short unsigned int*)queue.row(y);
	  while (!queue.empty() && (row == NULL)) {
	    queue.writer_process_row();
	    row = (short unsigned int*)queue.row(y);
	  }

	  // If it's still not available, something has gone wrong
	  if (row == NULL) {
	    std::cerr << "** Oh crap (y=" << y << ", num_rows=" << queue.num_rows() << " **" << std::endl;
	    exit(2);
	  }

	  if (can_free)
	    img->free_row(y);
	  ditherer.dither(row, temprow, y == img->height() - 1);
	  queue.free_row(y);

	  unsigned char *inp = temprow;
	  for (unsigned int x = 0; x < img->width(); x++) {
	    unsigned char r = *inp++;
	    unsigned char g = *inp++;
	    unsigned char b = *inp++;
	    ofs.put(b | ((g & 0x07) << 5));
	    ofs.put((g >> 3) | (r << 3));
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

    ofs.close();
    _is_open = false;

    std::cerr << "Done." << std::endl;
  }
}
