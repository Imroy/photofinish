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
#include <jxl/decode_cxx.h>

namespace fs = boost::filesystem;

namespace PhotoFinish {

  JXLreader::JXLreader(const fs::path filepath) :
    ImageReader(filepath)
  {}

  CMS::Format JXLreader::cmsformat(const JxlPixelFormat& pixelformat) {
    CMS::Format format;

    switch (pixelformat.num_channels) {
    case 2:
      format.set_extra_channels(1);
    case 1:
      format.set_channels(1);
      format.set_colour_model(CMS::ColourModel::Greyscale);
      break;

    case 4:
      format.set_extra_channels(1);
    case 3:
      format.set_channels(3);
      format.set_colour_model(CMS::ColourModel::RGB);
      break;

    default:
      break;
    }

    switch (pixelformat.data_type) {
    case JXL_TYPE_FLOAT:
      format.set_float();
      break;

    case JXL_TYPE_UINT8:
      format.set_8bit();
      break;

    case JXL_TYPE_UINT16:
      format.set_16bit();
      break;

    case JXL_TYPE_UINT32:
      format.set_32bit();
      break;

    default:
      break;
    }

    return format;
  }

  void JXLreader::getformats(JxlBasicInfo info, JxlPixelFormat& pixelformat, CMS::Format& cmsformat) {
    switch (info.bits_per_sample) {
    case 8:
      pixelformat.data_type = JXL_TYPE_UINT8;
      cmsformat.set_8bit();
      break;

    case 16:
      pixelformat.data_type = JXL_TYPE_UINT16;
      cmsformat.set_16bit();
      break;

    case 32:
      pixelformat.data_type = JXL_TYPE_UINT32;
      cmsformat.set_32bit();
      break;

    default:
      throw LibraryError("libjxl", "Unhandled bits_per_sample value");
    }
  }

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

    JxlBasicInfo info;
    JxlPixelFormat pixelformat;
    CMS::Format format;
    size_t outbuffer_size;
    uint8_t *outbuffer;
    Image::ptr image;

    size_t read_size = JxlDecoderSizeHintBasicInfo(decoder.get()), buffer_len = read_size;
    uint8_t *buffer = nullptr;
    std::streamsize total_read = 0;
    //    std::cerr << "\tRead 0 bytes";
    size_t unprocessed_len = 0, processed_len = 0;
    while (!ifs.eof()) {
      {
	uint8_t *old_buffer = buffer;
	buffer_len = unprocessed_len + read_size;
	buffer = new uint8_t[buffer_len];

	if (old_buffer != nullptr) {
	  if (unprocessed_len > 0)
	    memcpy(buffer, old_buffer + processed_len, unprocessed_len);
	  delete [] old_buffer;
	}
      }

      std::cerr << std::endl << "\treading " << read_size << " bytes into " << buffer_len << " byte buffer (+" << unprocessed_len << ")...";
      ifs.read((char*)buffer + unprocessed_len, read_size);
      auto buffer_read = ifs.gcount();
      std::cerr << std::endl << "\tread " << buffer_read << " bytes.";
      total_read += buffer_read;
      //      std::cerr << "\r\tRead " << total_read << " bytes.";

      if (JxlDecoderSetInput(decoder.get(), buffer, buffer_read) > 0)
	throw LibraryError("libjxl", "Could not set decoder input");

      auto status = JxlDecoderProcessInput(decoder.get());
      switch (status) {
      case JXL_DEC_ERROR:
	throw LibraryError("libjxl", "Decoder error");

      case JXL_DEC_NEED_MORE_INPUT:
	std::cerr << std::endl << "\tneeds more input...";
	break;

      case JXL_DEC_BASIC_INFO:
	if (JxlDecoderGetBasicInfo(decoder.get(), &info) > 0)
	  throw LibraryError("libjxl", "Could not get basic info");

	std::cerr << std::endl << "\t" << info.xsize << "×" << info.ysize
		  << ", " << info.num_color_channels << " channels"
		  << ", " << info.num_extra_channels << " extra channels"
		  << ", " << info.bits_per_sample << " bps.";

	getformats(info, pixelformat, format);

	pixelformat.num_channels = info.num_color_channels + info.num_extra_channels;
	pixelformat.endianness = JXL_NATIVE_ENDIAN;
	pixelformat.align = 0;

	format.set_channels(info.num_color_channels);
	format.set_extra_channels(info.num_extra_channels);
	format.set_premult_alpha(info.alpha_premultiplied);

	image = std::make_shared<Image>(info.xsize, info.ysize, format);

	read_size = 1024;
	break;

      case JXL_DEC_COLOR_ENCODING:
	std::cerr << std::endl << "\tgot colour encoding...";

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
	  for (uint32_t y = 0; y < info.ysize; y++) {
	    auto row = image->row(y);
	    memcpy(row->data(), outbuffer + (y * row_size), row_size);
	  }
	}
	break;

      default:
	break;
      }

      unprocessed_len = JxlDecoderReleaseInput(decoder.get());
      processed_len = buffer_len - unprocessed_len;
      std::cerr << std::endl << "\t" << unprocessed_len << " bytes unprocessed.";
    }
    std::cerr << std::endl;

    if (buffer != nullptr)
      delete [] buffer;

    return image;
  }

}; // namespace PhotoFinish
