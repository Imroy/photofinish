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
#include <unistd.h>
#include "ImageFile.hh"
#include "Image.hh"

PNGFile::PNGFile(const char* filepath) :
  _ImageFile(filepath)
{}

PNGFile::~PNGFile() {
  if (_filepath != NULL) {
    free((void*)_filepath);
    _filepath = NULL;
  }
}

struct pngrow_t {
  png_uint_32 row_num;
  png_bytep row_data;

  inline pngrow_t(png_uint_32 rn, png_bytep rd) : row_num(rn), row_data(rd) {}
};

struct callback_state {
  bool finished;
  std::queue<pngrow_t*> rowqueue;
  size_t rowlen;
  omp_lock_t *queue_lock;
  unsigned int width, height;
  Image *img;
  cmsHTRANSFORM transform;
};

void info_callback(png_structp png, png_infop info) {
  png_set_gamma(png, 1.0, 1.0);
  png_set_alpha_mode(png, PNG_ALPHA_PNG, 1.0);
  png_set_packing(png);
  png_set_swap(png);
  png_read_update_info(png, info);

  callback_state *cs = (callback_state*)png_get_progressive_ptr(png);

  int bit_depth, colour_type;
  //  fprintf(stderr, "info_callback: Getting header information...\n");
  png_get_IHDR(png, info, &cs->width, &cs->height, &bit_depth, &colour_type, NULL, NULL, NULL);
  fprintf(stderr, "%dx%d, %d bpp, type %d.\n", cs->width, cs->height, bit_depth, colour_type);

  cs->img = new Image(cs->width, cs->height);

  cmsUInt32Number cmsType;
  switch (colour_type) {
  case PNG_COLOR_TYPE_GRAY:
    cmsType = COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(bit_depth >> 3);
    cs->rowlen = cs->width * (bit_depth >> 3);
    break;
  case PNG_COLOR_TYPE_RGB:
    cmsType = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(bit_depth >> 3);
    cs->rowlen = cs->width * 3 * (bit_depth >> 3);
    break;
  default:
    fprintf(stderr, "unsupported PNG colour type %d\n", colour_type);
    exit(1);
  }

  cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
  cmsHPROFILE profile = NULL;

  if (png_get_valid(png, info, PNG_INFO_iCCP)) {
    fprintf(stderr, "Image has iCCP chunk.\n");
    char *profile_name;
    int compression_type;
    png_bytep profile_data;
    unsigned int profile_len;
    if (png_get_iCCP(png, info, &profile_name, &compression_type, &profile_data, &profile_len) == PNG_INFO_iCCP) {
      fprintf(stderr, "Loading ICC profile \"%s\" from file...\n", profile_name);
      profile = cmsOpenProfileFromMem(profile_data, profile_len);
    }
  }
  if (profile == NULL) {
    fprintf(stderr, "Using default sRGB profile...\n");
    profile = cmsCreate_sRGBProfile();
  }

  //  fprintf(stderr, "Creating colour transform...\n");
  cs->transform = cmsCreateTransform(profile, cmsType,
				     lab, IMAGE_TYPE,
				     INTENT_PERCEPTUAL, 0);
  cmsCloseProfile(lab);
  cmsCloseProfile(profile);
}

void row_callback(png_structp png, png_bytep row_data, png_uint_32 row_num, int pass) {
  callback_state *cs = (callback_state*)png_get_progressive_ptr(png);
  png_bytep new_row = (png_bytep)malloc(cs->rowlen);
  memcpy(new_row, row_data, cs->rowlen);
  pngrow_t *row = new pngrow_t(row_num, new_row);

  omp_set_lock(cs->queue_lock);
  cs->rowqueue.push(row);
  omp_unset_lock(cs->queue_lock);
}

void end_callback(png_structp png, png_infop info) {
  //  callback_state *cs = (callback_state*)png_get_progressive_ptr(png);
}

void process_row(callback_state* cs) {
  if (!cs->rowqueue.empty()) {
    omp_set_lock(cs->queue_lock);
    pngrow_t *row = cs->rowqueue.front();
    cs->rowqueue.pop();
    omp_unset_lock(cs->queue_lock);

    cmsDoTransform(cs->transform, row->row_data, cs->img->row(row->row_num), cs->width);
    free(row->row_data);
    delete row;
  }
}

void process_workqueue(callback_state* cs) {
  while (!(cs->rowqueue.empty() && cs->finished)) {
    while (cs->rowqueue.empty() && !cs->finished)
      usleep(100);
    process_row(cs);
  }
}

Image* PNGFile::read(void) {
  fprintf(stderr, "Opening file \"%s\"...\n", _filepath);
  FILE *fp = fopen(_filepath, "r");
  if (!fp) {
    fprintf(stderr, "PNGFile::read(): Could not open file \"%s\": %s\n", _filepath, strerror(errno));
    return NULL;
  }

  {
    unsigned char header[8];
    //    fprintf(stderr, "Reading header...\n");
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)) {
      fprintf(stderr, "PNGFile::read(): File \"%s\" is not a PNG file.\n", _filepath);
      return NULL;
    }
    fseek(fp, 0, SEEK_SET);
  }

  //  fprintf(stderr, "Creating read structure...\n");
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					   NULL, NULL, NULL);
  if (!png) {
    fprintf(stderr, "PNGFile::read(): Could not create PNG read structure.\n");
    return NULL;
  }

  //  fprintf(stderr, "Creating info structure...\n");
  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_read_struct(&png, (png_infopp)NULL, (png_infopp)NULL);
    fprintf(stderr, "PNGFile::read(): Could not create PNG info structure.\n");
    return NULL;
  }

  //  fprintf(stderr, "Setting jump point...\n");
  if (setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    fprintf(stderr, "PNGFile::read(): something went wrong reading the PNG.\n");
    return NULL;
  }

  callback_state cs;
  cs.queue_lock = (omp_lock_t*)malloc(sizeof(omp_lock_t));
  omp_init_lock(cs.queue_lock);
  cs.width = cs.height = 0;
  cs.transform = NULL;
  cs.finished = false;
#pragma omp parallel shared(cs)
  {
    int th_id = omp_get_thread_num();
    if (th_id == 0) {		// Master thread
      fprintf(stderr, "Reading PNG image and transforming into L*a*b* using %d threads...\n", omp_get_num_threads());
      png_set_progressive_read_fn(png, (void *)&cs, info_callback, row_callback, end_callback);
      png_byte buffer[1048576];
      size_t length;
      while ((length = fread(buffer, 1, 1048576, fp))) {
	png_process_data(png, info, buffer, length);
	while (cs.rowqueue.size() > 100)
	  process_row(&cs);
      }
      cs.finished = true;
      process_workqueue(&cs);	// Help finish off the transforming of image data
    } else {
      process_workqueue(&cs);	// Worker threads just process the workqueue
    }
  }
  omp_destroy_lock(cs.queue_lock);
  free(cs.queue_lock);
  cmsDeleteTransform(cs.transform);

  fprintf(stderr, "Done.\n");
  png_destroy_read_struct(&png, &info, NULL);
  fclose(fp);

  return cs.img;
}

bool PNGFile::write(Image* img, const Destination &d) {
  fprintf(stderr, "Opening file \"%s\"...\n", _filepath);
  FILE *fp = fopen(_filepath, "wb");
  if (!fp) {
    fprintf(stderr, "Could not open file \"%s\": %s\n", _filepath, strerror(errno));
    return false;
  }

  //  fprintf(stderr, "Creating write structure...\n");
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
					    NULL, NULL, NULL);
  if (!png) {
    fprintf(stderr, "Could not initialize PNG write structure.\n");
    return false;
  }

  //  fprintf(stderr, "Creating info structure...\n");
  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_write_struct(&png, (png_infopp)NULL);
    return false;
  }

  //  fprintf(stderr, "Setting jump point...\n");
  if (setjmp(png_jmpbuf(png))) {
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    fprintf(stderr, "something went wrong reading the PNG.\n");
    return false;
  }

  //  fprintf(stderr, "Initialising PNG IO...\n");
  png_init_io(png, fp);

  fprintf(stderr, "writing header for %dx%d %d-bit RGB PNG image...\n", img->width(), img->height(), d.depth());
  png_set_IHDR(png, info,
	       img->width(), img->height(), d.depth(), PNG_COLOR_TYPE_RGB,
	       PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_set_filter(png, 0, PNG_ALL_FILTERS);
  png_set_compression_level(png, Z_BEST_COMPRESSION);

  cmsHPROFILE profile;
  if (d.has_profile()) {
    profile = d.profile().profile();
    cmsUInt32Number len;
    cmsSaveProfileToMem(profile, NULL, &len);
    if (len > 0) {
      png_bytep profile_data = (png_bytep)malloc(len);
      if (cmsSaveProfileToMem(profile, profile_data, &len)) {
	fprintf(stderr, "Embedding profile \"%s\" (%d bytes)...\n", d.profile().name().c_str(), len);
	png_set_iCCP(png, info, d.profile().name().c_str(), 0, profile_data, len);
      }
    }
  } else {
    fprintf(stderr, "Using default sRGB profile...\n");
    profile = cmsCreate_sRGBProfile();
    png_set_sRGB_gAMA_and_cHRM(png, info, d.intent());
  }

  {
    time_t t = time(NULL);
    if (t > 0) {
      png_time ptime;
      png_convert_from_time_t(&ptime, t);
      fprintf(stderr, "Adding time chunk...\n");
      png_set_tIME(png, info, &ptime);
    }
  }

  png_bytepp png_rows = (png_bytepp)malloc(img->height() * sizeof(png_bytep));
  for (unsigned int y = 0; y < img->height(); y++)
    png_rows[y] = (png_bytep)malloc(img->width() * 3 * (d.depth() >> 3));

  png_set_rows(png, info, png_rows);

  cmsHPROFILE lab = cmsCreateLab4Profile(NULL);
  //  fprintf(stderr, "Creating colour transform...\n");
  cmsUInt32Number format = COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(d.depth() >> 3);
  cmsHTRANSFORM transform = cmsCreateTransform(lab, IMAGE_TYPE,
					       profile, format,
					       d.intent(), 0);
  cmsCloseProfile(lab);
  if (!d.has_profile())
    cmsCloseProfile(profile);

#pragma omp parallel
  {
#pragma omp master
    {
      fprintf(stderr, "Transforming image data from L*a*b* using %d threads...\n", omp_get_num_threads());
    }
  }
#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int y = 0; y < img->height(); y++)
    cmsDoTransform(transform, img->row(y), png_rows[y], img->width());

  cmsDeleteTransform(transform);

  fprintf(stderr, "Writing PNG image data...\n");
  png_write_png(png, info, PNG_TRANSFORM_SWAP_ENDIAN, NULL);

  for (unsigned int y = 0; y < img->height(); y++)
    free(png_rows[y]);
  free(png_rows);

  fprintf(stderr, "Done.\n");
  fclose(fp);

  return true;
}

