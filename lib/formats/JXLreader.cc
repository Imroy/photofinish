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
#include <stdexcept>
#include "ImageFile.hh"
#include "Image.hh"
#include "JXL.hh"
#include <jxl/decode_cxx.h>
#include <jxl/thread_parallel_runner_cxx.h>

namespace fs = boost::filesystem;

namespace PhotoFinish {

  JXLreader::JXLreader(const fs::path filepath) :
    ImageReader(filepath)
  {}

  Image::ptr JXLreader::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream ifs(_filepath, std::ios_base::in);
    if (ifs.fail())
      throw FileOpenError(_filepath.native());

    {
      unsigned char header[12];
      ifs.read((char*)header, 12);
      auto sig = JxlSignatureCheck(header, ifs.gcount());
      if (sig < JXL_SIG_CODESTREAM)
	throw FileContentError(_filepath.string(), "is not a JPEG XL codestream or container");
      ifs.seekg(0, std::ios_base::beg);
    }

    auto decoder = JxlDecoderMake(nullptr);
    if (JxlDecoderSubscribeEvents(decoder.get(),
				  JXL_DEC_BASIC_INFO
				  | JXL_DEC_COLOR_ENCODING
				  | JXL_DEC_FULL_IMAGE) > 0)
      throw LibraryError("libjxl", "Could not subscribe to decoder events");

    auto runner = JxlThreadParallelRunnerMake(nullptr, JxlThreadParallelRunnerDefaultNumWorkerThreads());
    if (JxlDecoderSetParallelRunner(decoder.get(),
				    JxlThreadParallelRunner,
				    runner.get()) > 0)
      throw LibraryError("libjxl", "Could not set parallel runners");

    JxlBasicInfo info;
    JxlPixelFormat pixelformat;
    CMS::Format format;
    size_t outbuffer_size;
    uint8_t *outbuffer;
    Image::ptr image;

    size_t read_size = JxlDecoderSizeHintBasicInfo(decoder.get()), buffer_size = 0;
    uint8_t *buffer = nullptr;
    std::streamsize total_proc = 0;
    std::cerr << "\tProcessed 0 bytes";

    JxlDecoderStatus status = JXL_DEC_NEED_MORE_INPUT;
    while (status != JXL_DEC_SUCCESS) {
      switch (status) {
      case JXL_DEC_ERROR:
	throw LibraryError("libjxl", "Decoder error");

      case JXL_DEC_NEED_MORE_INPUT:
	if (ifs.eof())
	  throw LibraryError("libjxl", "Decoder wants more input but file has ended");

	{
	  size_t unprocessed_len = 0, processed_len = 0;
	  if (buffer != nullptr) {
	    unprocessed_len = JxlDecoderReleaseInput(decoder.get());
	    processed_len = buffer_size - unprocessed_len;
	    total_proc += processed_len;
	    std::cerr << "\r\tProcessed " << format_byte_size(total_proc) << "   ";
	  }

	  uint8_t *old_buffer = buffer;
	  buffer_size = unprocessed_len + read_size;
	  buffer = new uint8_t[buffer_size];

	  if (old_buffer != nullptr) {
	    if (unprocessed_len > 0)
	      memcpy(buffer, old_buffer + processed_len, unprocessed_len);
	    delete [] old_buffer;
	  }

	  ifs.read((char*)buffer + unprocessed_len, read_size);
	  auto buffer_read = ifs.gcount();

	  if (JxlDecoderSetInput(decoder.get(), buffer, buffer_read) > 0)
	    throw LibraryError("libjxl", "Could not set decoder input");
	}

	break;

      case JXL_DEC_BASIC_INFO:
	if (JxlDecoderGetBasicInfo(decoder.get(), &info) > 0)
	  throw LibraryError("libjxl", "Could not get basic info");

	std::cerr << "\r\t" << info.xsize << "×" << info.ysize
		  << ", " << info.num_color_channels << " channels"
		  << ", " << info.num_extra_channels << " extra channels"
		  << ", " << info.bits_per_sample << " bps." << std::endl;

	getformats(info, pixelformat, format);

	image = std::make_shared<Image>(info.xsize, info.ysize, format);

	read_size = 65536;
	break;

      case JXL_DEC_COLOR_ENCODING:
	{
	  size_t profile_size;
	  if (JxlDecoderGetICCProfileSize(decoder.get(), &pixelformat,
					  JXL_COLOR_PROFILE_TARGET_DATA,
					  &profile_size) > 0)
	    throw LibraryError("libjxl", "Could not get ICC profile size");

	  uint8_t *profile_data = new uint8_t[profile_size];
	  if (JxlDecoderGetColorAsICCProfile(decoder.get(), &pixelformat,
					     JXL_COLOR_PROFILE_TARGET_DATA,
					     profile_data, profile_size) > 0)
	    throw LibraryError("libjxl", "Could not get ICC profile");

	  auto profile = std::make_shared<CMS::Profile>(profile_data, profile_size);
	  image->set_profile(profile);

	  uint8_t *profile_copy = new uint8_t[profile_size];
	  memcpy(profile_copy, profile_data, profile_size);
	  dest->set_profile("JPEG XL profile", profile_copy, profile_size);
	}

	read_size = 1048576;
	break;

      case JXL_DEC_NEED_IMAGE_OUT_BUFFER:
	if (JxlDecoderImageOutBufferSize(decoder.get(), &pixelformat, &outbuffer_size) > 0)
	  throw LibraryError("libjxl", "Could not get image outbuffer size");

	outbuffer = new uint8_t[outbuffer_size];

	if (JxlDecoderSetImageOutBuffer(decoder.get(), &pixelformat, outbuffer, outbuffer_size) > 0)
	  throw LibraryError("libjxl", "Could not set image outbuffer");
	break;

      case JXL_DEC_FULL_IMAGE:
	{
	  uint32_t row_size = info.xsize * (info.num_color_channels + info.num_extra_channels) * info.bits_per_sample >> 3;
#pragma omp parallel for schedule(dynamic, 1)
	  for (uint32_t y = 0; y < info.ysize; y++) {
	    image->check_row_alloc(y);
	    auto row = image->row(y);
	    memcpy(row->data(), outbuffer + (y * row_size), row_size);
	  }
	}
	break;

      case JXL_DEC_SUCCESS:
	continue;

      default:
	std::cerr << "\r\tStatus: " << status << std::endl;
	break;
      }

      status = JxlDecoderProcessInput(decoder.get());
    }
    std::cerr << std::endl;

    decoder.reset();
    if (outbuffer != nullptr)
      delete [] outbuffer;

    return image;
  }

}; // namespace PhotoFinish
