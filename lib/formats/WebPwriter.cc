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
#include <omp.h>
#include "ImageFile.hh"
#include "Exception.hh"
#include "WebP_ostream.hh"

namespace PhotoFinish {

  std::map<std::string, WebPPreset> WebP_presets = { std::make_pair("Default", WEBP_PRESET_DEFAULT),
						     std::make_pair("Picture", WEBP_PRESET_PICTURE),
						     std::make_pair("Photo", WEBP_PRESET_PHOTO),
						     std::make_pair("Drawing", WEBP_PRESET_DRAWING),
						     std::make_pair("Icon", WEBP_PRESET_ICON),
						     std::make_pair("Text", WEBP_PRESET_TEXT) };

  WebPwriter::WebPwriter(const fs::path filepath) :
    ImageWriter(filepath)
  {}

  CMS::Format WebPwriter::preferred_format(CMS::Format format) {
    format.set_colour_model(CMS::ColourModel::RGB);
    format.set_8bit();

    return format;
  }

  void WebPwriter::write(Image::ptr img, Destination::ptr dest, bool can_free) {
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
	    std::cerr << "\tUsing preset \"" << d.preset().get() << "\", quality=" << d.quality() << "." << std::endl;
	    ok = WebPConfigPreset(&config, i.second, d.quality());
	    if (!ok)
	      throw WebPError(ok);
	    need_init = false;
	    break;
	  }
      if (need_init) {
	ok = WebPConfigInit(&config);
	if (!ok)
	  throw WebPError(ok);
	config.quality = d.quality();
	std::cerr << "\tQuality=" << config.quality << std::endl;
      }

      if (d.lossless().defined()) {
	config.lossless = d.lossless();
	std::cerr << "\tLossless=" << config.lossless << std::endl;
      }

      if (d.method().defined()) {
	config.method = d.method();
	std::cerr << "\tMethod=" << config.method << std::endl;
      }
    } else {
      ok = WebPConfigInit(&config);
      if (!ok)
	throw WebPError(ok);
    }
    config.thread_level = 1;
    if (!WebPValidateConfig(&config))
      throw LibraryError("libwebp", "Bad config");

    WebPPicture pic;
    ok = WebPPictureInit(&pic);
    if (!ok)
      throw WebPError(ok);

    pic.width = img->width();
    pic.height = img->height();
    pic.use_argb = config.lossless;
    ok = WebPPictureAlloc(&pic);
    if (!ok)
      throw WebPError(ok);

    // libwebp takes its image data in one big chunk
    {
      std::cerr << "\tCopying image data to blob..." << std::endl;
      unsigned char *rgb = (unsigned char*)malloc(img->row_size() * img->height());
      for (unsigned int y = 0; y < img->height(); y++) {
	memcpy(rgb + (y * img->row_size()), img->row(y), img->row_size());
	if (can_free)
	  img->free_row(y);
      }

      if (img->format().extra_channels() > 0) {
	std::cerr << "\tImporting RGBA image data..." << std::endl;
	WebPPictureImportRGBA(&pic, rgb, img->row_size());
      } else {
	std::cerr << "\tImporting RGB image data..." << std::endl;
	WebPPictureImportRGB(&pic, rgb, img->row_size());
      }
      free(rgb);
    }

    fs::ofstream ofs(_filepath, std::ios_base::out);
    if (ofs.fail())
      throw FileOpenError(_filepath.native());

    webp_stream_writer wrt(&ofs, img->width(), img->height());
    wrt.add_icc(img->profile());
    wrt.add_exif(img->EXIFtags());
    wrt.add_xmp(img->XMPtags());

    pic.writer = webp_stream_writer_func;
    pic.custom_ptr = &wrt;

    std::cerr << "\tEncoding..." << std::endl;
    ok = WebPEncode(&config, &pic);
    std::cerr << std::endl;
    WebPPictureFree(&pic);

    unsigned int size = (int)ofs.tellp() - 8;
    ofs.seekp(4, std::ios_base::beg);
    unsigned char size_le[4];
    copy_le_to(size_le, size, 4);
    ofs.write((char*)size_le, 4);

    ofs.close();
    _is_open = false;

    /*
    if (!ok)
      throw WebPError(ok);
    */

    std::cerr << "Done." << std::endl;
  }

}
