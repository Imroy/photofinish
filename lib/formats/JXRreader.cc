/*
	Copyright 2013 Ian Tester

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

  JXRreader::JXRreader(const fs::path filepath) :
    ImageReader(filepath)
  {}

  Image::ptr JXRreader::read(Destination::ptr dest) {
    if (_is_open)
      throw FileOpenError("already open");
    _is_open = true;

    std::cerr << "Opening file " << _filepath << "..." << std::endl;
    long rc;

    PKCodecFactory *codec_factory;
    JXRcheck(PKCreateCodecFactory(&codec_factory, WMP_SDK_VERSION));

    PKImageDecode *decoder;
    JXRcheck(codec_factory->CreateDecoderFromFile(_filepath.c_str(), &decoder));

    int width, height;
    JXRcheck(decoder->GetSize(decoder, &width, &height));
    PKPixelFormatGUID pixel_format;
    JXRcheck(decoder->GetPixelFormat(decoder, &pixel_format));
    CMS::Format format = jxr_cms_format(pixel_format);
    std::cerr << "\t" << width << "×" << height << " " << format << std::endl;

    auto img = std::make_shared<Image>(width, height, format);

    float res_x, res_y;
    JXRcheck(decoder->GetResolution(decoder, &res_x, &res_y));
    img->set_resolution(res_x, res_y);

    {
      unsigned int profile_size;
      JXRcheck(decoder->GetColorContext(decoder, NULL, &profile_size));
      if (profile_size > 0) {
	unsigned char *profile_data = (unsigned char*)malloc(profile_size);
	std::cerr << "\tReading " << profile_size << " bytes of ICC profile..." << std::endl;
	JXRcheck(decoder->GetColorContext(decoder, profile_data, &profile_size));
	img->set_profile(std::make_shared<CMS::Profile>(profile_data, profile_size));
      }
    }

    decoder->WMP.wmiSCP.uAlphaMode = img->format().extra_channels();

    unsigned char *pixels;
    JXRcheck(PKAllocAligned((void**)&pixels, img->row_size() * img->height(), 128));

    PKRect rect;
    rect.X = 0;
    rect.Y = 0;
    rect.Width = img->width();
    rect.Height = img->height();

    decoder->Copy(decoder, &rect, pixels, img->row_size());
    for (unsigned int y = 0; y < img->height(); y++) {
      img->check_rowdata_alloc(y);
      memcpy(img->row(y), pixels + (y * img->row_size()), img->row_size());
      std::cerr << "\r\tRead " << y << " of " << img->height() << " rows";
    }
    std::cerr << "\r\tRead " << img->height() << " of " << img->height() << " rows" << std::endl;

    if (pixels)
      PKFreeAligned((void**)&pixels);

    if (decoder)
      decoder->Release(&decoder);
   
    if (codec_factory)
      codec_factory->Release(&codec_factory);

    return img;
  }

}
