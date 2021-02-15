/*
	Copyright 2021 Ian Tester

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
#include "ImageFile.hh"
#include "Image.hh"
#include "JXL.hh"
#include <jxl/encode_cxx.h>
#include <jxl/thread_parallel_runner_cxx.h>

namespace fs = boost::filesystem;

namespace PhotoFinish {

  JXLwriter::JXLwriter(const fs::path filepath) :
    ImageWriter(filepath)
  {}

  CMS::Format JXLwriter::preferred_format(CMS::Format format) {
    if (format.is_half() || format.is_double())
      format.set_float();

    if ((format.colour_model() != CMS::ColourModel::Greyscale)
	&& (format.colour_model() != CMS::ColourModel::RGB))
      format.set_colour_model(CMS::ColourModel::RGB);

    if (format.is_half() || format.is_double())
      format.set_float();

    format.unset_swapfirst();
    format.unset_endian16_swap();

    return format;
  }

  void JXLwriter::write(Image::ptr img, Destination::ptr dest, bool can_free) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    auto encoder = JxlEncoderMake(nullptr);

    auto runner = JxlThreadParallelRunnerMake(nullptr, JxlThreadParallelRunnerDefaultNumWorkerThreads());
    if (JxlEncoderSetParallelRunner(encoder.get(), JxlThreadParallelRunner, runner.get()) > 0)
      throw LibraryError("libjxl", "Could not set parallel runner");

    JxlPixelFormat pixel_format;
    JxlBasicInfo info = { .xsize = img->width(),
			  .ysize = img->height(),
			  .uses_original_profile = JXL_FALSE,
    };
    JxlColorEncoding colour_encoding;
    // TODO: fill in colour_encoding using image info
    JxlColorEncodingSetToSRGB(&colour_encoding, img->format().colour_model() == CMS::ColourModel::Greyscale);
    format_info(img->format(), pixel_format, info, colour_encoding);

    if (JxlEncoderSetBasicInfo(encoder.get(), &info) > 0)
      throw LibraryError("libjxl", "Could not set image basic info");

    if (JxlEncoderSetColorEncoding(encoder.get(), &colour_encoding) > 0)
      throw LibraryError("libjxl", "Could not set colour encoding");

    JxlEncoderOptions *encopts = JxlEncoderOptionsCreate(encoder.get(), nullptr);
    if (dest->jxl().defined()) {
      const D_JXL jxl = dest->jxl();

      if (jxl.lossless()) {
	if (JxlEncoderOptionsSetLossless(encopts, true) > 0)
	  throw LibraryError("libjxl", "Could not set lossless on encoder options");
      } else {
	if (JxlEncoderOptionsSetLossless(encopts, false) > 0)
	  throw LibraryError("libjxl", "Could not set lossy on encoder options");

	if (jxl.distance().defined())
	  if (JxlEncoderOptionsSetDistance(encopts, jxl.distance().get()) > 0)
	    throw LibraryError("libjxl", "Could not set distance on encoder options");
      }

      if (jxl.effort().defined())
	if (JxlEncoderOptionsSetEffort(encopts, jxl.effort().get()) > 0)
	  throw LibraryError("libjxl", "Could not set effort on encoder options");
    }

    size_t inbuffer_size = img->width() * img->height() * img->format().bytes_per_pixel();
    uint8_t *inbuffer = new uint8_t[inbuffer_size];

#pragma omp parallel for schedule(dynamic, 1)
    for (uint32_t y = 0; y < info.ysize; y++)
      memcpy(inbuffer + (y * img->row_size()), img->row(y)->data(), img->row_size());

    if (JxlEncoderAddImageFrame(encopts, &pixel_format, inbuffer, inbuffer_size) > 0)
      throw LibraryError("libjxl", "Could not add image frame");

    // Only encoding one frame
    JxlEncoderCloseInput(encoder.get());

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ofstream ofs;
    ofs.open(_filepath, std::ios_base::out);

    uint8_t *buffer = new uint8_t[1048576], *buffer_temp = buffer;
    size_t available = 1048576;

    std::cerr << "\tWrote 0 bytes.";

    JxlEncoderStatus status = JXL_ENC_NEED_MORE_OUTPUT;
    while (status != JXL_ENC_SUCCESS) {
      status = JxlEncoderProcessOutput(encoder.get(), &buffer_temp, &available);

      switch (status) {
      case JXL_ENC_SUCCESS:
	break;

      case JXL_ENC_ERROR:
	throw LibraryError("libjxl", "Could not process output");

      case JXL_ENC_NEED_MORE_OUTPUT:
	{
	  auto written = 1048576 - available;
	  std::cerr << "\r\tWrote " << format_byte_size(written) << ".  ";
	  ofs.write((char*)buffer, written);
	}

	available = 1048576;
	buffer_temp = buffer;
	break;

      case JXL_ENC_NOT_SUPPORTED:
      default:
	throw LibraryError("libjxl", "Something not supported");
      };
    }

    // Write last chunk of data
    {
      auto written = 1048576 - available;
      if (written > 0) {
	std::cerr << "\r\tWrote " << format_byte_size(written) << ".  " << std::endl;
	ofs.write((char*)buffer, written);
      }
    }

    ofs.close();
    _is_open = false;
    delete [] buffer;
  }

}; // namespace PhotoFinish
