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
#include <boost/algorithm/string.hpp>
#include <webp/encode.h>
#include <webp/decode.h>
#include <omp.h>
#include "ImageFile.hh"
#include "Exception.hh"

namespace PhotoFinish {

  std::map<std::string, WebPPreset> WebP_presets = { std::make_pair("Default", WEBP_PRESET_DEFAULT),
						     std::make_pair("Picture", WEBP_PRESET_PICTURE),
						     std::make_pair("Photo", WEBP_PRESET_PHOTO),
						     std::make_pair("Drawing", WEBP_PRESET_DRAWING),
						     std::make_pair("Icon", WEBP_PRESET_ICON),
						     std::make_pair("Text", WEBP_PRESET_TEXT) };

  //! WebP exception
  class WebPError : public std::exception {
  private:
    int _code;

  public:
    //! Constructor
    /*!
      \param c Error code
    */
    WebPError(int c) :
      _code(c)
    {}

    virtual const char* what() const throw() {
      switch (_code) {
      case 0:
	return std::string("No error").c_str();

      case 1:
	return std::string("Out of memory").c_str();

      case 2:
	return std::string("Bitstream out of memory").c_str();

      case 3:
	return std::string("NULL parameter").c_str();

      case 4:
	return std::string("Invalid configuration").c_str();

      case 5:
	return std::string("Bad dimension").c_str();

      case 6:
	return std::string("Partition is larger than 512 KiB").c_str();

      case 7:
	return std::string("Partition is larger than 16 MiB").c_str();

      case 8:
	return std::string("Bad write").c_str();

      case 9:
	return std::string("File is larger than 4 GiB").c_str();

      case 10:
	return std::string("User abort").c_str();

      }
      return std::string("Dunno").c_str();
    }
  };

  WebPfile::WebPfile(const fs::path filepath) :
    ImageFile(filepath)
  {}

  Image::ptr WebPfile::read(Destination::ptr dest) {
    throw Unimplemented("WebPfile", "read");
  }

  cmsUInt32Number WebPfile::preferred_type(cmsUInt32Number type) {
    type &= COLORSPACE_MASK;
    type |= COLORSPACE_SH(PT_RGB);
    type &= CHANNELS_MASK;
    type |= CHANNELS_SH(3);

    if (T_EXTRA(type) > 1) {
      type &= EXTRA_MASK;
      type |= EXTRA_SH(1);
    }

    type &= FLOAT_MASK;
    type &= BYTES_MASK;
    type |= BYTES_SH(1);

    return type;
  }

  int stream_writer(const uint8_t* data, size_t data_size, const WebPPicture* picture) {
    std::ostream *os = (std::ostream*)picture->custom_ptr;
    os->write((char*)data, data_size);
    std::cerr << "\r\tWritten " << os->tellp() << " bytes";

    return 1;
  }

  void WebPfile::write(Image::ptr img, Destination::ptr dest, bool can_free) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Preparing for file " << _filepath << "..." << std::endl;
    std::cerr << "\t" << img->width() << "×" << img->height() << std::endl;

    WebPConfig config;
    int ok;
    if (dest->webp().defined()) {
      D_WebP d = dest->webp();
      bool need_init = true;

      if (d.preset().defined())
	for (auto i : WebP_presets)
	  if (boost::iequals(i.first, d.preset().get())) {
	    std::cerr << "\tInitialising config with \"" << d.preset().get() << "\" preset." << std::endl;
	    ok = WebPConfigPreset(&config, i.second, d.quality());
	    if (!ok)
	      throw WebPError(ok);
	    need_init = false;
	  }
      if (need_init) {
	std::cerr << "\tInitialising config." << std::endl;
	ok = WebPConfigInit(&config);
	if (!ok)
	  throw WebPError(ok);
	config.quality = d.quality();
      }

      if (d.lossless().defined())
	config.lossless = d.lossless();

      if (d.method().defined())
	config.method = d.method();
    } else {
      std::cerr << "\tInitialising config." << std::endl;
      ok = WebPConfigInit(&config);
      if (!ok)
	throw WebPError(ok);
    }
    config.thread_level = 1;

    WebPPicture pic;
    ok = WebPPictureInit(&pic);
    if (!ok)
      throw WebPError(ok);

    pic.width = img->width();
    pic.height = img->height();
    ok = WebPPictureAlloc(&pic);
    if (!ok)
      throw WebPError(ok);

    // libwebp takes its image data in one big chunk
    {
      std::cerr << "\tCopying image data to blob..." << std::endl;
      unsigned char *rgb = (unsigned char*)malloc(img->row_size() * img->height());
      for (unsigned int y = 0; y < img->height(); y++) {
	memcpy(&rgb[y * img->row_size()], img->row(y), img->row_size());
	if (can_free)
	  img->free_row(y);
      }

      std::cerr << "\tConverting image data to YCbCr..." << std::endl;
      if (T_EXTRA(img->type()))
	WebPPictureImportRGBA(&pic, rgb, img->row_size());
      else
	WebPPictureImportRGB(&pic, rgb, img->row_size());
      free(rgb);
    }

    fs::ofstream ofs(_filepath, std::ios_base::out);
    if (ofs.fail())
      throw FileOpenError(_filepath.native());

    pic.writer = stream_writer;
    pic.custom_ptr = &ofs;

    std::cerr << "\tEncoding..." << std::endl;
    ok = WebPEncode(&config, &pic);
    std::cerr << std::endl;
    WebPPictureFree(&pic);

    ofs.close();
    _is_open = false;

    /*
    if (!ok)
      throw WebPError(ok);
    */

    std::cerr << "Done." << std::endl;
  }

}
