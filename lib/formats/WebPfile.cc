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

  struct stream_writer {
    std::ostream *stream;
    char chunk[4];
    unsigned int chunk_size;
    unsigned int next_chunk;

    // Data for the VP8X chunk
    unsigned int width, height;
    unsigned char *icc_data, *exif_data, *xmp_data;	// no IPTC?
    unsigned int icc_size, exif_size, xmp_size;

    stream_writer(std::ostream* s, unsigned int w, unsigned int h) :
      stream(s), chunk_size(0), next_chunk(12),
      width(w - 1), height(h - 1),
      icc_data(NULL), exif_data(NULL), xmp_data(NULL),
      icc_size(0), exif_size(0), xmp_size(0) {
      memcpy(chunk, "   ", 4);
    }

    inline void write_le16(unsigned short int x) {
      unsigned char b[2] = { (unsigned char)(x & 0xff), (unsigned char)(x >> 8) };
      stream->write((char*)&b, 2);
    }
    inline void write_le24(unsigned int x) {
      unsigned char b[3] = { (unsigned char)(x & 0xff), (unsigned char)((x >> 8) & 0xff), (unsigned char)((x >> 16) & 0xff) };
      stream->write((char*)&b, 3);
    }
    inline void write_le32(unsigned int x) {
      unsigned char b[4] = { (unsigned char)(x & 0xff), (unsigned char)((x >> 8) & 0xff), (unsigned char)((x >> 16) & 0xff), (unsigned char)(x >> 24) };
      stream->write((char*)&b, 4);
    }

    inline unsigned int read_le32(const unsigned char* data) {
      return data[0] | ((unsigned int)data[1] << 8) | ((unsigned int)data[2] << 16) | ((unsigned int)data[3] << 24);
    }

    void after_vp8(void) {
      if (exif_size) {
	std::cerr << "\tInserting EXIF chunk (" << exif_size << " bytes)..." << std::endl;
	stream->write("EXIF", 4);
	write_le32(exif_size);
	stream->write((char*)exif_data, exif_size);
	if (exif_size & 0x01)
	  stream->write("", 1);
      }
      if (xmp_size) {
	std::cerr << "\tInserting XMP chunk (" << xmp_size << " bytes)..." << std::endl;
	stream->write("XMP ", 4);
	write_le32(xmp_size);
	stream->write((char*)xmp_data, xmp_size);
	if (xmp_size & 0x01)
	  stream->write("", 1);
      }
    }

    int write(const unsigned char* data, size_t data_size, const WebPPicture* picture) {
      while (next_chunk < data_size) {
	std::string fourcc((char*)&data[next_chunk], data_size - next_chunk < 4 ? data_size - next_chunk : 4);
	unsigned int size = read_le32(data + next_chunk + 4);

	if (memcmp(chunk, "VP8 ", 4) == 0) {
	  // Write stuff after the image chunk
	  after_vp8();
	}

	std::cerr << "\tFound chunk: \"" << fourcc << "\", " << size << " bytes." << std::endl;

	if (memcmp(data + next_chunk, "VP8 ", 4) == 0) {
	  std::cerr << "\tInserting VP8X chunk..." << std::endl;
	  stream->write("VP8X", 4);
	  write_le32(10);
	  unsigned int flags = 0;
	  if (icc_size)
	    flags |= 1 << 2;
	  if (exif_size)
	    flags |= 1 << 4;
	  if (xmp_size)
	    flags |= 1 << 5;
	  write_le32(flags);
	  write_le24(width);
	  write_le24(height);

	  if (icc_size) {
	    std::cerr << "\tInserting ICCP chunk (" << icc_size << " bytes)..." << std::endl;
	    stream->write("ICCP", 4);
	    write_le32(icc_size);
	    stream->write((char*)icc_data, icc_size);
	    if (icc_size & 0x01)
	      stream->write("", 1);
	  }
	}

	memcpy(chunk, &data[next_chunk], 4);
	chunk_size = read_le32(data + next_chunk + 4);
	next_chunk += size + 8;
      }
      if (data_size) {
	std::cerr << "\tWriting " << data_size << " bytes..." << std::endl;
	stream->write((char*)data, data_size);
	next_chunk -= data_size;
	//      std::cerr << "\r\tWritten " << stream->tellp() << " bytes";
      }

      if ((next_chunk == 0) && (memcmp(chunk, "VP8 ", 4) == 0)) {
	// Write stuff after the image chunk
	after_vp8();
      }

      return 1;
    }
  };

  int stream_writer_func(const uint8_t* data, size_t data_size, const WebPPicture* picture) {
    stream_writer *wrt = (stream_writer*)picture->custom_ptr;
    return wrt->write(data, data_size, picture);
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

    stream_writer wrt(&ofs, img->width(), img->height());
    cmsSaveProfileToMem(img->profile(), NULL, &wrt.icc_size);
    if (wrt.icc_size > 0) {
      wrt.icc_data = (unsigned char*)malloc(wrt.icc_size);
      cmsSaveProfileToMem(img->profile(), wrt.icc_data, &wrt.icc_size);
    }
    {
      Exiv2::Blob blob;
      Exiv2::ExifParser::encode(blob, Exiv2::littleEndian, img->EXIFtags());
      wrt.exif_size = blob.size();
      unsigned char *data = wrt.exif_data = (unsigned char*)malloc(wrt.exif_size);
      for (auto i : blob)
	*data++ = i;
    }
    {
      std::string xmp;
      Exiv2::XmpParser::encode(xmp, img->XMPtags());
      wrt.xmp_size = xmp.length() + 1;
      wrt.xmp_data = (unsigned char*)malloc(wrt.xmp_size);
      memcpy(wrt.xmp_data, xmp.c_str(), wrt.xmp_size);
    }

    pic.writer = stream_writer_func;
    pic.custom_ptr = &wrt;

    std::cerr << "\tEncoding..." << std::endl;
    ok = WebPEncode(&config, &pic);
    std::cerr << std::endl;
    WebPPictureFree(&pic);

    // Fix the size of the file since we've written our own chunks
    unsigned int size = (int)ofs.tellp() - 8;
    ofs.seekp(4, std::ios_base::beg);
    wrt.write_le32(size);

    ofs.close();
    _is_open = false;

    /*
    if (!ok)
      throw WebPError(ok);
    */

    std::cerr << "Done." << std::endl;
  }

}
