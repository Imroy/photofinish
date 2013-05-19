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
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    fs::ifstream ifs(_filepath, std::ios_base::in);
    if (ifs.fail())
      throw FileOpenError(_filepath.native());

    WebPDecBuffer decbuffer;
    WebPInitDecBuffer(&decbuffer);
    decbuffer.colorspace = MODE_RGBA;		// No way to find out if the file has an alpha channel?
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
	if (img == NULL)
	  img = new Image(width, height, COLORSPACE_SH(PT_RGB) | BYTES_SH(1) | CHANNELS_SH(3) | EXTRA_SH(1));
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

    //    img->

    return Image::ptr(img);
  }

  cmsUInt32Number WebPfile::preferred_type(cmsUInt32Number type) {
    type &= COLORSPACE_MASK;
    type |= COLORSPACE_SH(PT_RGB);
    type &= CHANNELS_MASK;
    type |= CHANNELS_SH(3);

    type &= FLOAT_MASK;
    type &= BYTES_MASK;
    type |= BYTES_SH(1);

    return type;
  }

  inline void copy_le_to(unsigned char* dest, unsigned int value, unsigned char length) {
    for (unsigned char i = 0; i < length; i++) {
      dest[i] = (unsigned char)(value & 0xff);
      value >>= 8;
    }
  }

  inline unsigned int read_le32(const unsigned char* data) {
    return data[0] | ((unsigned int)data[1] << 8) | ((unsigned int)data[2] << 16) | ((unsigned int)data[3] << 24);
  }

  //! A custom writer for libwebp that writes using a std::ostream object
  /*!
    This class is so large because libwebp does not handle metadata *at all*.
    So we have to keep track of RIFF chunks as the encoder emits them and
    insert our own, even modifying one of the chunks (VP8X).
  */
  class webp_stream_writer {
  private:
    std::ostream *stream;
    char chunk[4];
    unsigned int chunk_size;
    unsigned int next_chunk;

    // Data for the VP8X chunk
    bool need_vp8x;
    unsigned int width, height;
    unsigned char *icc_data, *exif_data, *xmp_data;	// no IPTC?
    unsigned int icc_size, exif_size, xmp_size;

  public:
    //! Constructor
    /*!
      \param s Pointer to a std::ostream derivative.
      \param w,h Width and height of the image
    */
    webp_stream_writer(std::ostream* s, unsigned int w, unsigned int h) :
      stream(s), chunk_size(0), next_chunk(12),
      need_vp8x(false), width(w - 1), height(h - 1),
      icc_data(NULL), exif_data(NULL), xmp_data(NULL),
      icc_size(0), exif_size(0), xmp_size(0) {
      memcpy(chunk, "   ", 4);
    }

    ~webp_stream_writer() {
      if (icc_data)
	free(icc_data);
      if (exif_data)
	free(exif_data);
      if (xmp_data)
	free(xmp_data);
    }

    //! Add an LCMS2 profile to be written
    void add_icc(cmsHPROFILE profile) {
      cmsSaveProfileToMem(profile, NULL, &icc_size);
      if (icc_size > 0) {
	icc_data = (unsigned char*)malloc(icc_size);
	cmsSaveProfileToMem(profile, icc_data, &icc_size);
      }
      need_vp8x = true;
    }

    //! Add a set of EXIF tags to be written
    void add_exif(const Exiv2::ExifData& exif) {
      Exiv2::Blob blob;
      Exiv2::ExifParser::encode(blob, Exiv2::littleEndian, exif);
      exif_size = blob.size();
      unsigned char *data = exif_data = (unsigned char*)malloc(exif_size);
      for (auto i : blob)
	*data++ = i;
      need_vp8x = true;
    }

    //! Add a set of XMP tags to be written
    void add_xmp(const Exiv2::XmpData& xmp) {
      std::string s;
      Exiv2::XmpParser::encode(s, xmp);
      xmp_size = s.length() + 1;
      xmp_data = (unsigned char*)malloc(xmp_size);
      memcpy(xmp_data, s.c_str(), xmp_size);
      need_vp8x = true;
    }

    //! Write a RIFF chunk
    void write_chunk(const char *fourcc, const unsigned char* data, unsigned int length) {
	stream->write(fourcc, 4);
	unsigned char l[4];
	copy_le_to(l, length, 4);
	stream->write((char*)l, 4);
	stream->write((char*)data, length);
	if (length & 0x01)
	  stream->put(0);
    }

    void modify_vp8x(unsigned char* data) {
      if (icc_size)
	*data |= 1 << 2;
      if (exif_size)
	*data |= 1 << 4;
      if (xmp_size)
	*data |= 1 << 5;
    }

    //! Write a block of data from the encoder
    int write(unsigned char* data, size_t data_size, const WebPPicture* picture) {
      while (next_chunk < data_size) {
	unsigned int size = read_le32(data + next_chunk + 4);
	std::cerr << "\tFound chunk: \"" << std::string((char*)data + next_chunk, 4) << "\", " << size << " bytes." << std::endl;

	if (need_vp8x && (memcmp(data + next_chunk, "VP8X", 4) == 0)) {
	  std::cerr << "\tModifying VP8X chunk..." << std::endl;
	  modify_vp8x(data + next_chunk + 8);
	  need_vp8x = false;
	}

	if (need_vp8x && (memcmp(data + next_chunk, "VP8 ", 4) == 0)) {
	  std::cerr << "\tInserting VP8X chunk..." << std::endl;
	  unsigned char vp8x[10];
	  modify_vp8x(vp8x);
	  copy_le_to(vp8x + 4, width, 3);
	  copy_le_to(vp8x + 7, height, 3);
	  write_chunk("VP8X", vp8x, 10);

	  if (icc_size) {
	    std::cerr << "\tInserting ICCP chunk (" << icc_size << " bytes)..." << std::endl;
	    write_chunk("ICCP", icc_data, icc_size);
	  }
	  need_vp8x = false;
	}

	memcpy(chunk, data + next_chunk, 4);
	chunk_size = read_le32(data + next_chunk + 4);
	next_chunk += size + (size & 0x01) + 8;
      }
      if (data_size) {
	std::cerr << "\tWriting " << data_size << " bytes..." << std::endl;
	stream->write((char*)data, data_size);
	next_chunk -= data_size;
	//      std::cerr << "\r\tWritten " << stream->tellp() << " bytes";
      }

      // Write stuff after a chunk
      if (next_chunk == 0) {
	if (memcmp(chunk, "VP8X", 4) == 0) {
	  std::cerr << "\tInserting ICCP chunk (" << icc_size << " bytes)..." << std::endl;
	  write_chunk("ICCP", icc_data, icc_size);
	}
	if (memcmp(chunk, "VP8 ", 4) == 0) {
	  if (exif_size) {
	    std::cerr << "\tInserting EXIF chunk (" << exif_size << " bytes)..." << std::endl;
	    write_chunk("EXIF", exif_data, exif_size);
	  }
	  if (xmp_size) {
	    std::cerr << "\tInserting XMP chunk (" << xmp_size << " bytes)..." << std::endl;
	    write_chunk("XMP ", xmp_data, xmp_size);
	  }
	}
      }

      return 1;
    }
  };

  //! Wrapper around the webp_stream_writer class
  int webp_stream_writer_func(const uint8_t* data, size_t data_size, const WebPPicture* picture) {
    webp_stream_writer *wrt = (webp_stream_writer*)picture->custom_ptr;
    return wrt->write(const_cast<unsigned char*>(data), data_size, picture);
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
