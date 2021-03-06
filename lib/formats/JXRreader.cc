/*
	Copyright 2014-2019 Ian Tester

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
#include <JXRGlue.h>
#undef min
#undef max

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include "ImageFile.hh"
#include "Image.hh"
#include "JXR.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  // jxrlib doesn't provide many functions/methods for getting metadata from a file.
  // So copy some code and make it into a generic macro
#define jxr_metadata_size(decoder, name) decoder->WMP.wmiDEMisc.u##name##ByteCount
#define jxr_metadata_data(decoder, name, data) \
    struct WMPStream* s = decoder->pStream;		\
    size_t curr_pos;							\
    JXRcheck(s->GetPos(s, &curr_pos));				\
    JXRcheck(s->SetPos(s, decoder->WMP.wmiDEMisc.u##name##Offset));	\
    JXRcheck(s->Read(s, data, decoder->WMP.wmiDEMisc.u##name##ByteCount)); \
    JXRcheck(s->SetPos(s, curr_pos));


  JXRreader::JXRreader(const fs::path filepath) :
    ImageReader(filepath)
  {}

  Image::ptr JXRreader::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    Image::ptr img;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    long rc;

    PKCodecFactory *codec_factory = nullptr;
    try {
      JXRcheck(PKCreateCodecFactory(&codec_factory, WMP_SDK_VERSION));

      PKImageDecode *decoder = nullptr;
      try {
	JXRcheck(codec_factory->CreateDecoderFromFile(_filepath.c_str(), &decoder));

	{
	  int width, height;
	  JXRcheck(decoder->GetSize(decoder, &width, &height));
	  PKPixelFormatGUID pixel_format;
	  JXRcheck(decoder->GetPixelFormat(decoder, &pixel_format));
	  CMS::Format format = jxr_cms_format(pixel_format);

	  std::cerr << "\t" << width << "×" << height << " " << format << std::endl;
	  img = std::make_shared<Image>(width, height, format);
	}

	{
	  float res_x, res_y;
	  JXRcheck(decoder->GetResolution(decoder, &res_x, &res_y));
	  img->set_resolution(res_x, res_y);
	}

	{
	  unsigned int profile_size = jxr_metadata_size(decoder, ColorProfile);
	  if (profile_size > 0) {
	    unsigned char *profile_data = new unsigned char[profile_size];
	    std::cerr << "\tLoading ICC profile (" << format_byte_size(profile_size) << ")..." << std::endl;
	    jxr_metadata_data(decoder, ColorProfile, profile_data);
	    img->set_profile(std::make_shared<CMS::Profile>(profile_data, profile_size));
	    delete [] profile_data;
	  }
	}

	{
	  unsigned int xmp_size = jxr_metadata_size(decoder, XMPMetadata);
	  if (xmp_size > 0) {
	    unsigned char *xmp_data = new unsigned char[xmp_size];
	    std::cerr << "\tLoading XMP metadata (" << format_byte_size(xmp_size) << ")..." << std::endl;
	    jxr_metadata_data(decoder, XMPMetadata, xmp_data);
	    Exiv2::XmpParser::decode(img->XMPtags(), std::string((char*)xmp_data));
	    delete [] xmp_data;
	  }
	}

	{
	  unsigned int exif_size = jxr_metadata_size(decoder, EXIFMetadata);
	  if (exif_size > 0) {
	    unsigned char *exif_data = new unsigned char[exif_size];
	    std::cerr << "\tLoading EXIF metadata (" << format_byte_size(exif_size) << ")..." << std::endl;
	    jxr_metadata_data(decoder, EXIFMetadata, exif_data);
	    Exiv2::ExifParser::decode(img->EXIFtags(), exif_data, exif_size);
	    delete [] exif_data;
	  }
	}

	{
	  unsigned int iptc_size = jxr_metadata_size(decoder, IPTCNAAMetadata);
	  if (iptc_size > 0) {
	    unsigned char *iptc_data = new unsigned char[iptc_size];
	    std::cerr << "\tLoading IPTC metadata (" << format_byte_size(iptc_size) << ")..." << std::endl;
	    jxr_metadata_data(decoder, IPTCNAAMetadata, iptc_data);
	    Exiv2::IptcParser::decode(img->IPTCtags(), iptc_data, iptc_size);
	    delete [] iptc_data;
	  }
	}

	decoder->WMP.wmiSCP.uAlphaMode = img->format().extra_channels();

	unsigned char *pixels = nullptr;
	try {
	  JXRcheck(PKAllocAligned((void**)&pixels, img->row_size() * img->height(), 128));

	  {
	    PKRect rect;
	    rect.X = 0;
	    rect.Y = 0;
	    rect.Width = img->width();
	    rect.Height = img->height();
	    decoder->Copy(decoder, &rect, pixels, img->row_size());
	  }

	  for (unsigned int y = 0; y < img->height(); y++) {
	    img->check_row_alloc(y);
	    memcpy(img->row(y)->data(), pixels + (y * img->row_size()), img->row_size());
	    std::cerr << "\r\tRead " << y << " of " << img->height() << " rows";
	  }
	  std::cerr << "\r\tRead " << img->height() << " of " << img->height() << " rows" << std::endl;

	} catch (std::exception& ex) {
	  std::cout << ex.what() << std::endl;
	}

	if (pixels)
	  PKFreeAligned((void**)&pixels);

      } catch (std::exception& ex) {
	std::cout << ex.what() << std::endl;
      }

      if (decoder)
	decoder->Release(&decoder);

    } catch (std::exception& ex) {
      std::cout << ex.what() << std::endl;
    }

    if (codec_factory)
      codec_factory->Release(&codec_factory);

    return img;
  }

}
