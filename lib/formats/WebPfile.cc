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
#include "WebP_ostream.hh"

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
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream ifs(_filepath, std::ios_base::in);
    if (ifs.fail())
      throw FileOpenError(_filepath.native());

    WebPDecBuffer decbuffer;
    WebPInitDecBuffer(&decbuffer);

    decbuffer.colorspace = MODE_RGB;
    CMS::Format format = CMS::Format::RGB8();
    CMS::Profile::ptr profile;
    Exiv2::ExifData EXIFtags;
    Exiv2::XmpData XMPtags;
    {
      char fourcc[4];
      unsigned char size_le[4];
      ifs.read(fourcc, 4);
      if (memcmp(fourcc, "RIFF", 4) != 0)
	throw FileContentError(_filepath.native(), "Not a RIFF file");

      ifs.read((char*)size_le, 4);
      unsigned int file_size = read_le32(size_le);

      ifs.read(fourcc, 4);
      if (memcmp(fourcc, "WEBP", 4) != 0)
	throw FileContentError(_filepath.native(), "Not a WebP file");

      unsigned int next_chunk = 0;
      unsigned int chunk_size;
      do {
	ifs.read(fourcc, 4);
	ifs.read((char*)size_le, 4);
	chunk_size = read_le32(size_le);
	next_chunk += 8 + chunk_size + (chunk_size & 0x01);
	std::cerr << "\tFound \"" << std::string(fourcc, 4) << "\" chunk, " << chunk_size << " bytes long." << std::endl;

	if (memcmp(fourcc, "VP8X", 4) == 0) {
	  unsigned char flags = ifs.get();
	  if (flags & (1 << 4)) {
	    decbuffer.colorspace = MODE_RGBA;
	    format.set_extra_channels(1);
	  }
	}
	if (memcmp(fourcc, "ICCP", 4) == 0) {
	  unsigned char *profile_data = (unsigned char*)malloc(chunk_size);
	  ifs.read((char*)profile_data, chunk_size);
	  profile = std::make_shared<CMS::Profile>(profile_data, chunk_size);
	  if (profile != NULL) {
	    std::string profile_name = profile->read_info(cmsInfoDescription, "en", cmsNoCountry);
	    if (profile_name.length() > 0)
	      dest->set_profile(profile_name, profile_data, chunk_size);
	    else
	      dest->set_profile("WebP", profile_data, chunk_size);

	    std::cerr << "\tRead embedded profile \"" << dest->profile()->name().get() << "\" (" << chunk_size << " bytes)." << std::endl;
	  }
	}
	if (memcmp(fourcc, "EXIF", 4) == 0) {
	  unsigned char *data = (unsigned char*)malloc(chunk_size);
	  ifs.read((char*)data, chunk_size);
	  std::cerr << "\tReading EXIF tags..." << std::endl;
	  Exiv2::ExifParser::decode(EXIFtags, data, chunk_size);
	  free(data);
	}
	if (memcmp(fourcc, "XMP ", 4) == 0) {
	  char *data = (char*)malloc(chunk_size);
	  ifs.read(data, chunk_size);
	  std::string s(data, chunk_size);
	  std::cerr << "\tReading XMP tags..." << std::endl;
	  Exiv2::XmpParser::decode(XMPtags, s);
	  free(data);
	}

	ifs.seekg(next_chunk + 12, std::ios_base::beg);
	ifs.peek();
      } while (ifs.good() && (next_chunk < file_size));
      ifs.seekg(0, std::ios_base::beg);
    }
    WebPIDecoder* idec = WebPINewDecoder(&decbuffer);

    unsigned char buffer[1048576];
    size_t length;
    Image *img = NULL;
    int width, height, y = 0, last_y, stride;
    unsigned char *rowdata;
    do {
      ifs.read((char*)buffer, 1048576);
      length = ifs.gcount();
      int status = WebPIAppend(idec, buffer, length);

      rowdata = WebPIDecGetRGB(idec, &last_y, &width, &height, &stride);
      if (rowdata != NULL) {
	if (img == NULL) {
	  std::cerr << "\t" << width << "×" << height << " RGB" << (format.extra_channels() > 0 ? "A" : "") << std::endl;
	  img = new Image(width, height, format);
	}
	while (y < last_y) {
	  memcpy(img->row(y), rowdata, stride);
	  std::cerr << "\r\tRead " << (y + 1) << " of " << height << " rows";
	  y++;
	  rowdata += stride;
	}
      }

      if (status != VP8_STATUS_SUSPENDED)
	break;
    } while (!ifs.eof());
    std::cerr << "\r\tRead " << height << " of " << height << " rows." << std::endl;

    WebPIDelete(idec);

    if (profile != NULL)
      img->set_profile(profile);

    for (auto ei : EXIFtags)
      img->EXIFtags().add(ei);

    for (auto xi : XMPtags)
      img->XMPtags().add(xi);

    return Image::ptr(img);
  }

  CMS::Format WebPfile::preferred_format(CMS::Format format) {
    format.set_colour_model(CMS::ColourModel::RGB);
    format.set_channels(3);
    format.set_8bit();

    return format;
  }

  //! Wrapper around the webp_stream_writer class
  int webp_stream_writer_func(const uint8_t* data, size_t data_size, const WebPPicture* picture) {
    webp_stream_writer *wrt = (webp_stream_writer*)picture->custom_ptr;
    return wrt->write(const_cast<unsigned char*>(data), data_size);
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
