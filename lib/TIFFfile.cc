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

  Image::ptr TIFFfile::read(void) const {
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
    std::cerr << "\tImage is " << width << "x" << height << std::endl;
    Image::ptr img(new Image(width, height));

    uint16 bit_depth, photometric;
    TIFFcheck(GetField(tiff, TIFFTAG_BITSPERSAMPLE, &bit_depth));
    std::cerr << "\tImage has a depth of " << bit_depth << std::endl;
    TIFFcheck(GetField(tiff, TIFFTAG_PHOTOMETRIC, &photometric));

    cmsUInt32Number cmsType;
    switch (photometric) {
    case PHOTOMETRIC_MINISBLACK:
      cmsType = COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(bit_depth >> 3);
      img->set_greyscale();
      break;
    case PHOTOMETRIC_RGB:
      cmsType = COLORSPACE_SH(PT_RGB) | DOSWAP_SH(1) | CHANNELS_SH(3) | BYTES_SH(bit_depth >> 3);
      break;
    default:
      std::cerr << "** unsupported TIFF photometric interpretation " << photometric << " **" << std::endl;
      exit(1);
    }

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHPROFILE profile = NULL;

    uint32 profile_len;
    void *profile_data;
    if (TIFFGetField(tiff, TIFFTAG_ICCPROFILE, &profile_len, &profile_data) == 1) {
      std::cerr << "\tImage has ICC profile." << std::endl;
      profile = cmsOpenProfileFromMem(profile_data, profile_len);
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

    cmsHTRANSFORM transform = cmsCreateTransform(profile, cmsType,
						 lab, IMAGE_TYPE,
						 INTENT_PERCEPTUAL, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    transform_queue queue(img, T_CHANNELS(cmsType), transform);

#pragma omp parallel shared(queue)
    {
      int th_id = omp_get_thread_num();
      if (th_id == 0) {		// Master thread
	std::cerr << "\tReading TIFF image and transforming into L*a*b* using " << omp_get_num_threads() << " threads..." << std::endl;
	for (unsigned int y = 0; y < height; y++) {
	  tdata_t buffer = _TIFFmalloc(TIFFScanlineSize(tiff));
	  TIFFcheck(ReadScanline(tiff, buffer, y));
	  std::cerr << "\r\tRead " << (y + 1) << " of " << height << " rows ("
		    << queue.num_rows() << " in queue for colour transformation)   ";

	  queue.add(y, buffer);

	  while (queue.num_rows() > 100) {
	    queue.reader_process_row();
	    std::cerr << "\r\tRead " << (y + 1) << " of " << height << " rows ("
		      << queue.num_rows() << " in queue for colour transformation)   ";
	  }
	}
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

    TIFFFlush(tiff);
    fb.close();

    return img;
  }

  void TIFFfile::write(Image::ptr img, const Destination &dest, const Tags &tags) const {
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
    if (dest.depth().defined())
      depth = dest.depth();
    TIFFcheck(SetField(tiff, TIFFTAG_BITSPERSAMPLE, depth));

    cmsUInt32Number intent = INTENT_PERCEPTUAL;	// Default value
    if (dest.intent().defined())
      intent = dest.intent();

    cmsHPROFILE profile = NULL;
    if (dest.profile().defined() && dest.profile().filepath().defined())
      profile = cmsOpenProfileFromFile(dest.profile().filepath()->c_str(), "r");

    if (profile == NULL) {
      if (img->is_colour()) {
	std::cerr << "\tUsing default sRGB profile." << std::endl;
	profile = cmsCreate_sRGBProfile();
      } else {
	std::cerr << "\tUsing default greyscale profile." << std::endl;
	cmsToneCurve *gamma = cmsBuildGamma(NULL, 2.2);
	profile = cmsCreateGrayProfile(cmsD50_xyY(), gamma);
	cmsFreeToneCurve(gamma);
      }
    }

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

    cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
    cmsHTRANSFORM transform = cmsCreateTransform(lab, IMAGE_TYPE,
						 profile, cmsTempType,
						 intent, 0);
    cmsCloseProfile(lab);
    cmsCloseProfile(profile);

    transform_queue queue(img, T_CHANNELS(cmsTempType), transform);
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
	  free(row);

	  std::cerr << "\r\tTransformed " << y + 1 << " of " << img->height() << " rows ("
		    << queue.num_rows() << " left)  ";
	}
	free(tiff_row);
      } else {	// Other thread(s) transform the image data
	while (!queue.empty())
	  queue.writer_process_row();
      }
    }
    cmsDeleteTransform(transform);
    std::cerr << std::endl;

    TIFFFlush(tiff);
    fb.close();

    std::cerr << "Done." << std::endl;

    tags.embed(_filepath);

  }

}
