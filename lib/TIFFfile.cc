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
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <tiffio.h>
#include <tiffio.hxx>
#include "ImageFile.hh"
#include "TransformQueue.hh"
#include "Ditherer.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  TIFFfile::TIFFfile(const fs::path filepath) :
    ImageFile(filepath)
  {}

#define TIFFcheck(x) if ((rc = TIFF##x) != 1) throw LibraryError("libtiff", "TIFF" #x " returned " + rc)

  Image::ptr TIFFfile::read(Destination::ptr dest) const {
    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream fb(_filepath, std::ios_base::in);
    if (fb.fail())
      throw FileOpenError(_filepath.native());

    TIFF *tiff = TIFFStreamOpen("", &fb);
    if (tiff == NULL)
      throw FileOpenError(_filepath.native());

    int rc;
    uint32 width, height;
    TIFFcheck(GetField(tiff, TIFFTAG_IMAGEWIDTH, &width));
    TIFFcheck(GetField(tiff, TIFFTAG_IMAGELENGTH, &height));
    std::cerr << "\tImage is " << width << "×" << height << std::endl;
    Image::ptr img(new Image(width, height));

    uint16 bit_depth, channels, photometric;
    TIFFcheck(GetField(tiff, TIFFTAG_BITSPERSAMPLE, &bit_depth));
    dest->set_depth(bit_depth);
    std::cerr << "\tImage has a depth of " << bit_depth << std::endl;
    TIFFcheck(GetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &channels));
    TIFFcheck(GetField(tiff, TIFFTAG_PHOTOMETRIC, &photometric));

    cmsUInt32Number cmsType = CHANNELS_SH(channels) | BYTES_SH(bit_depth >> 3);
    switch (photometric) {
    case PHOTOMETRIC_MINISWHITE:
      cmsType |= FLAVOR_SH(1);
    case PHOTOMETRIC_MINISBLACK:
      cmsType |= COLORSPACE_SH(PT_GRAY);
      img->set_greyscale();
      break;

    case PHOTOMETRIC_RGB:
      cmsType |= COLORSPACE_SH(PT_RGB);
      break;

    case PHOTOMETRIC_SEPARATED:
      cmsType |= COLORSPACE_SH(PT_CMYK);
      break;

    default:
      std::cerr << "** unsupported TIFF photometric interpretation " << photometric << " **" << std::endl;
      exit(1);
    }

    {
      uint16 extra_count, *extra_types;
      if (TIFFGetField(tiff, TIFFTAG_EXTRASAMPLES, &extra_count, &extra_types) == 1) {
	cmsType |= EXTRA_SH(extra_count & 0x07);
	for (int i = 0; i < extra_count; i++) {
	  std::cerr << "\tImage has an ";
	  switch (extra_types[i]) {
	  case EXTRASAMPLE_UNSPECIFIED: std::cerr << "unspecified ";
	    break;
	  case EXTRASAMPLE_ASSOCALPHA: std::cerr << "associated alpha ";
	    break;
	  case EXTRASAMPLE_UNASSALPHA: std::cerr << "unassociated alpha ";
	    break;
	  default: std::cerr << "unknown ";
	  }
	  std::cerr << "extra channel." << std::endl;
	}
      }
    }

    {
      float xres, yres;
      short unsigned int resunit;
      if ((TIFFGetField(tiff, TIFFTAG_XRESOLUTION, &xres) == 1)
	  && (TIFFGetField(tiff, TIFFTAG_YRESOLUTION, &yres) == 1)
	  && (TIFFGetField(tiff, TIFFTAG_RESOLUTIONUNIT, &resunit) == 1)) {
	switch (resunit) {
	case RESUNIT_INCH:
	  img->set_resolution(xres, yres);
	  break;
	case RESUNIT_CENTIMETER:
	  img->set_resolution((double)xres * 2.54, (double)yres * 2.54);
	  break;
	default:
	  std::cerr << "** unknown resolution unit " << resunit << " **" << std::endl;
	}
	if (img->xres().defined() && img->yres().defined())
	  std::cerr << "\tImage has resolution of " << img->xres() << "×" << img->yres() << " PPI." << std::endl;
      }
    }

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHPROFILE profile = NULL;

    uint32 profile_len;
    void *profile_data;
    if (TIFFGetField(tiff, TIFFTAG_ICCPROFILE, &profile_len, &profile_data) == 1) {
      profile = cmsOpenProfileFromMem(profile_data, profile_len);
      void *data_copy = malloc(profile_len);
      memcpy(data_copy, profile_data, profile_len);

      unsigned int profile_name_len;
      if ((profile_name_len = cmsGetProfileInfoASCII(profile, cmsInfoDescription, "en", cmsNoCountry, NULL, 0)) > 0) {
	char *profile_name = (char*)malloc(profile_name_len);
	cmsGetProfileInfoASCII(profile, cmsInfoDescription, "en", cmsNoCountry, profile_name, profile_name_len);
	dest->set_profile(profile_name, data_copy, profile_len);
	free(profile_name);
      } else
	dest->set_profile("TIFFTAG_ICCPROFILE", data_copy, profile_len);
      std::cerr << "\tRead embedded profile \"" << dest->profile()->name().get() << "\" (" << profile_len << " bytes)" << std::endl;
    }
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
	tdata_t buffer = _TIFFmalloc(TIFFScanlineSize(tiff));
	std::cerr << "\tReading TIFF image and transforming into L*a*b* using " << omp_get_num_threads() << " threads..." << std::endl;
	for (unsigned int y = 0; y < height; y++) {
	  TIFFcheck(ReadScanline(tiff, buffer, y));
	  std::cerr << "\r\tRead " << (y + 1) << " of " << height << " rows ("
		    << queue.num_rows() << " in queue for colour transformation)   ";

	  queue.add_copy(y, buffer);

	  while (queue.num_rows() > 100) {
	    queue.reader_process_row();
	    std::cerr << "\r\tRead " << (y + 1) << " of " << height << " rows ("
		      << queue.num_rows() << " in queue for colour transformation)   ";
	  }
	}
	free(buffer);
	queue.set_finished();
	while (!queue.empty()) {
	  queue.reader_process_row();
	  std::cerr << "\r\tRead " << height << " of " << height << " rows ("
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

    TIFFClose(tiff);
    fb.close();

    std::cerr << "Done." << std::endl;
    return img;
  }

  void TIFFfile::write(Image::ptr img, Destination::ptr dest) const {
    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ofstream fb;
    fb.open(_filepath, std::ios_base::out);

    TIFF *tiff = TIFFStreamOpen("", &fb);
    if (tiff == NULL)
      throw FileOpenError(_filepath.native());

    int rc;

    TIFFcheck(SetField(tiff, TIFFTAG_SUBFILETYPE, 0));
    TIFFcheck(SetField(tiff, TIFFTAG_IMAGEWIDTH, img->width()));
    TIFFcheck(SetField(tiff, TIFFTAG_IMAGELENGTH, img->height()));
    TIFFcheck(SetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT));
    TIFFcheck(SetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE));
    TIFFcheck(SetField(tiff, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL));
    TIFFcheck(SetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG));

    // For some reason none of this information shows up in the written TIFF file
    /*
    TIFFcheck(SetField(tiff, TIFFTAG_SOFTWARE, "PhotoFinish"));
    TIFFcheck(SetField(tiff, TIFFTAG_DOCUMENTNAME, _filepath.filename().c_str()));
    {
      char *hostname = (char*)malloc(101);
      if (gethostname(hostname, 100) == 0) {
	std::cerr << "\tSetting hostcomputer tag to \"" << hostname << "\"" << std::endl;
	TIFFcheck(SetField(tiff, TIFFTAG_HOSTCOMPUTER, hostname));
      }
    }
    {
      time_t t = time(NULL);
      tm *lt = localtime(&t);
      if (lt != NULL) {
	char *datetime = (char*)malloc(20);
	strftime(datetime, 20, "%Y:%m:%d %H:%M:%S", lt);
	std::cerr << "\tSetting datetime tag to \"" << datetime << "\"" << std::endl;
	TIFFcheck(SetField(tiff, TIFFTAG_DATETIME, datetime));
      }
    }
    */

    if (img->xres().defined()) {
      TIFFcheck(SetField (tiff, TIFFTAG_XRESOLUTION, (float)img->xres()));
      TIFFcheck(SetField (tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH));
    }
    if (img->yres().defined()) {
      TIFFcheck(SetField (tiff, TIFFTAG_YRESOLUTION, (float)img->yres()));
      TIFFcheck(SetField (tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH));
    }

    cmsUInt32Number cmsTempType;
    if (img->is_colour()) {
      TIFFcheck(SetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB));
      TIFFcheck(SetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 3));
      cmsTempType = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(2);
    } else {
      TIFFcheck(SetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK));
      TIFFcheck(SetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 1));
      cmsTempType = COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(2);
    }

    int depth = 8;	// Default value
    if (dest->depth().defined())
      depth = dest->depth();
    TIFFcheck(SetField(tiff, TIFFTAG_BITSPERSAMPLE, depth));

    cmsUInt32Number intent = INTENT_PERCEPTUAL;	// Default value
    if (dest->intent().defined())
      intent = dest->intent();

    cmsHPROFILE profile = NULL;
    if (dest->profile()) {
      profile = dest->profile()->profile();
      if (dest->profile()->has_data()) {
	std::cerr << "\tEmbedding profile \"" << dest->profile()->name().get() << " from data (" << dest->profile()->data_size() << " bytes)." << std::endl;
	TIFFcheck(SetField(tiff, TIFFTAG_ICCPROFILE, dest->profile()->data_size(), dest->profile()->data()));
      }
    } else {
      if (profile == NULL)
	profile = this->default_profile(cmsTempType);

      if (profile != NULL) {
	cmsUInt32Number len;
	cmsSaveProfileToMem(profile, NULL, &len);
	if (len > 0) {
	  void *profile_data = malloc(len);
	  if (cmsSaveProfileToMem(profile, profile_data, &len)) {
	    std::cerr << "\tEmbedding profile (" << len << " bytes)." << std::endl;
	    TIFFcheck(SetField(tiff, TIFFTAG_ICCPROFILE, len, profile_data));
	  }
	}
      }
    }

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHTRANSFORM transform = cmsCreateTransform(lab, IMAGE_TYPE,
						 profile, cmsTempType,
						 intent, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    transform_queue queue(dest, img, T_CHANNELS(cmsTempType), transform);
    for (unsigned int y = 0; y < img->height(); y++)
      queue.add(y);

#pragma omp parallel shared(queue)
    {
      int th_id = omp_get_thread_num();
      if (th_id == 0) {		// Master thread
	std::cerr << "\tTransforming image data from L*a*b* using " << omp_get_num_threads() << " threads." << std::endl;

	unsigned char *tiff_row = (unsigned char*)malloc(img->width() * T_CHANNELS(cmsTempType) * (depth >> 3));
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

	    if (depth == 8) {
	      ditherer.dither(row, tiff_row, y == img->height() - 1);
	      TIFFWriteScanline(tiff, tiff_row, y, 0);
	    } else
	      TIFFWriteScanline(tiff, row, y, 0);
	  }
	  queue.free_row(y);

	  std::cerr << "\r\tTransformed " << y + 1 << " of " << img->height() << " rows ("
		    << queue.num_rows() << " left)  ";
	}
	std::cerr << std::endl;
	free(tiff_row);
      } else {	// Other thread(s) transform the image data
	while (!queue.empty())
	  queue.writer_process_row();
      }
    }
    cmsDeleteTransform(transform);

    TIFFClose(tiff);
    fb.close();

    std::cerr << "Done." << std::endl;
  }

}
