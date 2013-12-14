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

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include "ImageFile.hh"
#include "Image.hh"
#include "JXR.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  static int jxr_qp_table[12][6] = {
    { 67, 79, 86, 72, 90, 98 },
    { 59, 74, 80, 64, 83, 89 },
    { 53, 68, 75, 57, 76, 83 },
    { 49, 64, 71, 53, 70, 77 },
    { 45, 60, 67, 48, 67, 74 },
    { 40, 56, 62, 42, 59, 66 },
    { 33, 49, 55, 35, 51, 58 },
    { 27, 44, 49, 28, 45, 50 },
    { 20, 36, 42, 20, 38, 44 },
    { 13, 27, 34, 13, 28, 34 },
    {  7, 17, 21,  8, 17, 21 },
    {  2,  5,  6,  2,  5,  6 }
  };

  JXRwriter::JXRwriter(const fs::path filepath) :
    ImageWriter(filepath)
  {}

  CMS::Format JXRwriter::preferred_format(CMS::Format format) {
    if ((format.colour_model() != CMS::ColourModel::Greyscale)
	&& (format.colour_model() != CMS::ColourModel::CMYK)) {
      format.set_colour_model(CMS::ColourModel::RGB);
    }

    format.set_packed();

    format.set_extra_channels(0);
    format.set_premult_alpha();

    return format;
  }

  void JXRwriter::write(Image::ptr img, Destination::ptr dest, bool can_free) {
    long rc;

    PKFactory *factory = NULL;
    JXRcheck(PKCreateFactory(&factory, PK_SDK_VERSION));

    PKCodecFactory *codec_factory = NULL;
    JXRcheck(PKCreateCodecFactory(&codec_factory, WMP_SDK_VERSION));

    struct WMPStream *stream;
    JXRcheck(factory->CreateStreamFromFilename(&stream, _filepath.c_str(), "wb"));

    PKImageEncode *encoder = NULL;
    JXRcheck(codec_factory->CreateCodec(&IID_PKImageWmpEncode, (void**)&encoder));
   
    CWMIStrCodecParam wmiSCP;
    memset(&wmiSCP, 0, sizeof(wmiSCP));
    wmiSCP.bVerbose = FALSE;
    wmiSCP.bdBitDepth = BD_LONG;
    wmiSCP.bfBitstreamFormat = FREQUENCY;
    wmiSCP.bProgressiveMode = TRUE;    
    wmiSCP.sbSubband = SB_ALL;
    wmiSCP.uAlphaMode = img->format().extra_channels();
    wmiSCP.bBlackWhite = FALSE;

    if (img->format().colour_model() == CMS::ColourModel::Greyscale)
      wmiSCP.cfColorFormat = Y_ONLY;
    else {
      wmiSCP.cfColorFormat = YUV_444;
      if (dest->jxr().defined()) {
	std::string sub = dest->jxr().subsampling();
	if (boost::iequals(sub, "420"))
	  wmiSCP.cfColorFormat = YUV_420;
	else if (boost::iequals(sub, "422"))
	  wmiSCP.cfColorFormat = YUV_422;
	else if (boost::iequals(sub, "CMYK"))
	  wmiSCP.cfColorFormat = CMYK;
      }
    }
       
    float iq_float = (dest->jxr().defined() ? dest->jxr().imageq() : 100) / 100.0f;
   
    if (iq_float == 1.0f)    
      wmiSCP.olOverlap = OL_NONE;
    else {
      wmiSCP.olOverlap = iq_float > 0.4f ? OL_ONE : OL_TWO;
      if (dest->jxr().defined() && dest->jxr().overlaping().defined()) {
	std::string ol = dest->jxr().overlaping();
	if (boost::iequals(ol, "none"))
	  wmiSCP.olOverlap = OL_NONE;
	else if (boost::iequals(ol, "one"))
	  wmiSCP.olOverlap = OL_ONE;
	else if (boost::iequals(ol, "two"))
	  wmiSCP.olOverlap = OL_TWO;
      }
    }
   
    if (iq_float == 1.0f)
      wmiSCP.uiDefaultQPIndex = 1;
    else {
      float iq;
      if (iq_float > 0.8f)
	iq = 0.8f + (iq_float - 0.8f) * 1.5f;
      else
	iq = iq_float;

      int qi = 10.0f * iq;
      float qf = 10.0f * iq - (float)qi;
           
      int *qp_row = jxr_qp_table[qi];

      wmiSCP.uiDefaultQPIndex    = (U8)(0.5f + qp_row[0] * (1.0f - qf) + (qp_row + 6)[0] * qf);
      wmiSCP.uiDefaultQPIndexU   = (U8)(0.5f + qp_row[1] * (1.0f - qf) + (qp_row + 6)[1] * qf);
      wmiSCP.uiDefaultQPIndexV   = (U8)(0.5f + qp_row[2] * (1.0f - qf) + (qp_row + 6)[2] * qf);
      wmiSCP.uiDefaultQPIndexYHP = (U8)(0.5f + qp_row[3] * (1.0f - qf) + (qp_row + 6)[3] * qf);
      wmiSCP.uiDefaultQPIndexUHP = (U8)(0.5f + qp_row[4] * (1.0f - qf) + (qp_row + 6)[4] * qf);
      wmiSCP.uiDefaultQPIndexVHP = (U8)(0.5f + qp_row[5] * (1.0f - qf) + (qp_row + 6)[5] * qf);
    }

    wmiSCP.cNumOfSliceMinus1H = wmiSCP.cNumOfSliceMinus1V = 0;
    if (dest->jxr().defined() && dest->jxr().tilesize().defined()) {
      int tile_size = dest->jxr().tilesize();
       
      int i = 0;
      unsigned int p = 0;
      for (i = 0; i < MAX_TILES; i++) {
	wmiSCP.uiTileY[i] = tile_size / 16;
	p += tile_size;
	if (p >= img->height())
	  break;
      }
      
      wmiSCP.cNumOfSliceMinus1H = i;
       
      p = 0;
       
      for (i = 0; i < MAX_TILES; i++) {
	wmiSCP.uiTileX[i] = tile_size / 16;
	p += tile_size;
	if (p >= img->width())
	  break;
      }
       
      wmiSCP.cNumOfSliceMinus1V = i;
    }
   
    JXRcheck(encoder->Initialize(encoder, stream, &wmiSCP, sizeof(wmiSCP)));
   
    std::cerr << "JXRwriter::write: format=" << img->format() << " (" << (unsigned int)img->format() << ")" << std::endl;
    PKPixelFormatGUID pixel_format = jxr_pixel_format(img->format());
    JXRcheck(encoder->SetPixelFormat(encoder, pixel_format));

    JXRcheck(encoder->SetSize(encoder, img->width(), img->height()));
    JXRcheck(encoder->SetResolution(encoder, img->xres(), img->yres()));

    unsigned char *pixels;
    JXRcheck(PKAllocAligned((void**)&pixels, img->row_size() * img->height(), 128));
    for (unsigned int y = 0; y < img->height(); y++) {
      memcpy(pixels + (y * img->row_size()), img->row(y), img->row_size());
      std::cerr << "\r\tWritten " << y + 1 << " of " << img->height() << " rows";
    }
    std::cerr << "\r\tWritten " << img->height() << " of " << img->height() << " rows" << std::endl;
    JXRcheck(encoder->WritePixels(encoder, img->height(), pixels, img->row_size()));
    PKFreeAligned((void**)&pixels);

    if (encoder)
      encoder->Release(&encoder);
   
    if (codec_factory)
      codec_factory->Release(&codec_factory);
   
    if (factory)
      factory->Release(&factory);
  }
}
