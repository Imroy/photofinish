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
#include "Exception.hh"
#include "JXR.hh"

namespace PhotoFinish {

  jxr_format_subst JXR_format_table = {
    FmtPair(TYPE_GRAY_8, &GUID_PKPixelFormat8bppGray),

    FmtPair(TYPE_GRAY_16, &GUID_PKPixelFormat16bppGray),
    // No grey-alpha

    FmtPair(TYPE_BGR_8, &GUID_PKPixelFormat24bppBGR),
    FmtPair(TYPE_RGB_8, &GUID_PKPixelFormat24bppRGB),

    FmtPair(TYPE_BGRA_8, &GUID_PKPixelFormat32bppBGR),
    FmtPair(TYPE_BGRA_8, &GUID_PKPixelFormat32bppBGRA),
    //    FmtPair(?, &GUID_PKPixelFormat32bppPBGRA),
    FmtPair(TYPE_GRAY_FLT, &GUID_PKPixelFormat32bppGrayFloat),
    FmtPair(TYPE_RGBA_8, &GUID_PKPixelFormat32bppRGB),
    FmtPair(TYPE_RGBA_8, &GUID_PKPixelFormat32bppRGBA),
    //    FmtPair(?, &GUID_PKPixelFormat32bppPRGBA),
  };

  const PKPixelFormatGUID& jxr_pixel_format(unsigned int n) {
    for (auto i : JXR_format_table)
      if (i.first == n)
	return *i.second;

    throw cmsTypeError("Unhandled format", n);
  }

  CMS::Format jxr_cms_format(const PKPixelFormatGUID &g) {
    long rc;

    PKPixelInfo pi;
    pi.pGUIDPixFmt = &g;
    JXRcheck(PixelFormatLookup(&pi, LOOKUP_FORWARD));
    CMS::Format f;
    switch (pi.cfColorFormat) {
    case Y_ONLY:
      f.set_colour_model(CMS::ColourModel::Greyscale);
      break;

    case YUV_420:
    case YUV_422:
    case YUV_444:
    case CF_RGB:
      f.set_colour_model(CMS::ColourModel::RGB);
      break;

    case CMYK:
      f.set_colour_model(CMS::ColourModel::CMYK);
      break;

    default:
      std::cerr << "Unhandled JPEG XR colour format " << (int)pi.cfColorFormat << std::endl;
      break;
    }

    switch (pi.bdBitDepth) {
    case BD_8:
      f.set_8bit();
      break;

    case BD_16:
      f.set_16bit();
      break;

    case BD_32:
      f.set_32bit();
      break;

    case BD_16F:
      f.set_half();
      break;

    case BD_32F:
      f.set_float();
      break;

    default:
      std::cerr << "Unhandled JPEG XR bit depth " << (int)pi.bdBitDepth << std::endl;
    }

    if (pi.grBit & PK_pixfmtHasAlpha)
      f.set_extra_channels(1);

    if (pi.grBit & PK_pixfmtPreMul)
      f.set_premult_alpha();

    if (pi.grBit & PK_pixfmtBGR)
      f.set_swap();

    return f;
  }

} // namespace PhotoFinish
