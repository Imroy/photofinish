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

#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <string.h>
#include "CMS.hh"

namespace fs = boost::filesystem;

namespace CMS {

  /*
    class Profile
  */

  Profile::Profile()
    : _profile(cmsCreateProfilePlaceholder(NULL))
  {
  }

  Profile::Profile(const Profile& other) :
    _profile(NULL)
  {
    cmsUInt32Number length;
    cmsSaveProfileToMem(other._profile, NULL, &length);
    if (length) {
      void *data = (cmsHPROFILE)malloc(length);
      cmsSaveProfileToMem(other._profile, data, &length);
      _profile = cmsOpenProfileFromMem(data, length);
      free(data);
    }
  }

  Profile::Profile(fs::path filepath)
    : _profile(cmsOpenProfileFromFile(filepath.generic_string().c_str(), "r"))
  {
  }

  Profile::Profile(const void* data, cmsUInt32Number size)
    : _profile(cmsOpenProfileFromMem(data, size))
  {
  }

  Profile::Profile(std::istream stream)
    : _profile(NULL)
  {
  }

  Profile::~Profile() {
    if (_profile)
      cmsCloseProfile(_profile);
  }

  Profile::ptr Profile::Lab4(void) {
    return std::make_shared<Profile>(cmsCreateLab4Profile(NULL));
  }
      
  Profile::ptr Profile::sRGB(void) {
    return std::make_shared<Profile>(cmsCreate_sRGBProfile());
  }
      
  Profile::ptr Profile::sGrey(void) {
    cmsCIExyY D65;
    cmsWhitePointFromTemp(&D65, 6504);
    double Parameters[5] = {
      2.4,                  // Gamma
      1.0 / 1.055,          // a
      0.055 / 1.055,        // b
      1.0 / 12.92,          // c
      0.04045,              // d
    };
    // y = (x >= d ? (a*x + b)^Gamma : c*x)
    cmsToneCurve *gamma = cmsBuildParametricToneCurve(NULL, 4, Parameters);
    Profile::ptr profile = std::make_shared<Profile>(cmsCreateGrayProfile(&D65, gamma));
    cmsFreeToneCurve(gamma);

    profile->write_MLU(cmsSigProfileDescriptionTag, "en", "AU", "sGrey built-in");
    profile->write_MLU(cmsSigCopyrightTag, "en", "AU", "No copyright, use freely");

    return profile;
  }

  void Profile::write_MLU(cmsTagSignature sig, std::string language, std::string country, std::string text) {
    cmsMLU *MLU = cmsMLUalloc(NULL, 1);
    if (MLU != NULL) {
      if (cmsMLUsetASCII(MLU, language.c_str(), country.c_str(), text.c_str()))
	cmsWriteTag(_profile, sig, MLU);
      cmsMLUfree(MLU);
    }
  }

  void Profile::write_MLU(cmsTagSignature sig, std::string language, std::string country, std::wstring text) {
    cmsMLU *MLU = cmsMLUalloc(NULL, 1);
    if (MLU != NULL) {
      if (cmsMLUsetWide(MLU, language.c_str(), country.c_str(), text.c_str()))
	cmsWriteTag(_profile, sig, MLU);
      cmsMLUfree(MLU);
    }
  }

  std::string Profile::read_info(cmsInfoType type, std::string language, std::string country) const {
    const char *lc = language.length() > 0 ? language.c_str() : cmsNoLanguage;
    const char *cc = country.length() > 0 ? country.c_str() : cmsNoCountry;
    unsigned int text_len;
    if ((text_len = cmsGetProfileInfoASCII(_profile, type, lc, cc, NULL, 0)) > 0) {
      char *text = (char*)malloc(text_len);
      cmsGetProfileInfoASCII(_profile, type, lc, cc, text, text_len);

      std::string s(text);
      free(text);
      return s;
    }
    return "";
  }

  std::wstring Profile::read_info_wide(cmsInfoType type, std::string language, std::string country) const {
    const char *lc = language.length() > 0 ? language.c_str() : cmsNoLanguage;
    const char *cc = country.length() > 0 ? country.c_str() : cmsNoCountry;
    unsigned int text_len;
    if ((text_len = cmsGetProfileInfo(_profile, type, lc, cc, NULL, 0)) > 0) {
      wchar_t *text = (wchar_t*)malloc(text_len * sizeof(wchar_t));
      cmsGetProfileInfo(_profile, type, lc, cc, text, text_len);

      std::wstring ws(text);
      free(text);
      return ws;
    }
    return (wchar_t*)"";
  }

  void Profile::set_description(std::string language, std::string country, std::string text) {
    this->write_MLU(cmsSigProfileDescriptionTag, language, country, text);
  }

  void Profile::set_description(std::string language, std::string country, std::wstring text) {
    this->write_MLU(cmsSigProfileDescriptionTag, language, country, text);
  }

  std::string Profile::description(std::string language, std::string country) const {
    return this->read_info(cmsInfoDescription, language, country);
  }

  std::wstring Profile::description_wide(std::string language, std::string country) const {
    return this->read_info_wide(cmsInfoDescription, language, country);
  }

  void Profile::set_manufacturer(std::string language, std::string country, std::string text) {
    this->write_MLU(cmsSigDeviceMfgDescTag, language, country, text);
  }

  void Profile::set_manufacturer(std::string language, std::string country, std::wstring text) {
    this->write_MLU(cmsSigDeviceMfgDescTag, language, country, text);
  }

  std::string Profile::manufacturer(std::string language, std::string country) const {
    return this->read_info(cmsInfoManufacturer, language, country);
  }

  std::wstring Profile::manufacturer_wide(std::string language, std::string country) const {
    return this->read_info_wide(cmsInfoManufacturer, language, country);
  }

  void Profile::set_model(std::string language, std::string country, std::string text) {
    this->write_MLU(cmsSigDeviceModelDescTag, language, country, text);
  }

  void Profile::set_model(std::string language, std::string country, std::wstring text) {
    this->write_MLU(cmsSigDeviceModelDescTag, language, country, text);
  }

  std::string Profile::model(std::string language, std::string country) const {
    return this->read_info(cmsInfoModel, language, country);
  }

  std::wstring Profile::model_wide(std::string language, std::string country) const {
    return this->read_info_wide(cmsInfoModel, language, country);
  }

  void Profile::set_copyright(std::string language, std::string country, std::string text) {
    this->write_MLU(cmsSigCopyrightTag, language, country, text);
  }

  void Profile::set_copyright(std::string language, std::string country, std::wstring text) {
    this->write_MLU(cmsSigCopyrightTag, language, country, text);
  }

  std::string Profile::copyright(std::string language, std::string country) const {
    return this->read_info(cmsInfoCopyright, language, country);
  }

  std::wstring Profile::copyright_wide(std::string language, std::string country) const {
    return this->read_info_wide(cmsInfoCopyright, language, country);
  }

  void Profile::save_to_mem(void* &dest, unsigned int &size) const {
    cmsSaveProfileToMem(_profile, NULL, &size);
    if (size > 0) {
      dest = malloc(size);
      cmsSaveProfileToMem(_profile, dest, &size);
    } else
      dest = NULL;
  }



  /*
    class enum ColourModel
  */

  std::ostream& operator<< (std::ostream& out, ColourModel model) {
    switch (model) {
    case ColourModel::Greyscale:	out << "Greyscale";
      break;
    case ColourModel::RGB:		out << "RGB";
      break;
    case ColourModel::CMY:		out << "CMY";
      break;
    case ColourModel::CMYK:		out << "CMYK";
      break;
    case ColourModel::YCbCr:		out << "Y'CbCr";
      break;
    case ColourModel::YUV:		out << "YUV";
      break;
    case ColourModel::XYZ:		out << "XYZ";
      break;
    case ColourModel::Lab:		out << "L*a*b*";
      break;
    case ColourModel::YUVK:		out << "YUVK";
      break;
    case ColourModel::HSV:		out << "HSV";
      break;
    case ColourModel::HLS:		out << "HLS";
      break;
    case ColourModel::Yxy:		out << "Yxy";
      break;
    case ColourModel::LabV2:		out << "L*a*b* v2";
      break;

    default:
      out << "[unknown (" << (int)model << ")]";
    }
    return out;
  }



  /*
    class Format
  */
  Format::Format(cmsUInt32Number f, bool pa)
    : _format(f),
      _premult_alpha(pa)
  {
  }

  Format::Format()
    : _format(0),
      _premult_alpha(false)
  {
  }

  Format Format::Grey8(void) {
    return Format(COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(1));
  }

  Format Format::Grey16(void) {
    return Format(COLORSPACE_SH(PT_GRAY) | CHANNELS_SH(1) | BYTES_SH(2));
  }

  Format Format::RGB8(void) {
    return Format(COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(1));
  }

  Format Format::RGB16(void) {
    return Format(COLORSPACE_SH(PT_RGB) | CHANNELS_SH(3) | BYTES_SH(2));
  }

  Format Format::CMYK8(void) {
    return Format(COLORSPACE_SH(PT_CMYK) | CHANNELS_SH(4) | BYTES_SH(1));
  }

  Format Format::LabFloat(void) {
    return Format(FLOAT_SH(1) | COLORSPACE_SH(PT_Lab) | CHANNELS_SH(3) | BYTES_SH(4));
  }

  Format Format::LabDouble(void) {
    return Format(FLOAT_SH(1) | COLORSPACE_SH(PT_Lab) | CHANNELS_SH(3) | BYTES_SH(0));
  }

// Some masks for manipulating LCMS "types"
#define FLOAT_MASK      (0xffffffff ^ FLOAT_SH(1))
#define OPTIMIZED_MASK  (0xffffffff ^ OPTIMIZED_SH(1))
#define COLORSPACE_MASK (0xffffffff ^ COLORSPACE_SH(31))
#define SWAPFIRST_MASK  (0xffffffff ^ SWAPFIRST_SH(1))
#define FLAVOR_MASK     (0xffffffff ^ FLAVOR_SH(1))
#define PLANAR_MASK     (0xffffffff ^ PLANAR_SH(1))
#define ENDIAN16_MASK   (0xffffffff ^ ENDIAN16_SH(1))
#define DOSWAP_MASK     (0xffffffff ^ DOSWAP_SH(1))
#define EXTRA_MASK      (0xffffffff ^ EXTRA_SH(7))
#define CHANNELS_MASK   (0xffffffff ^ CHANNELS_SH(15))
#define BYTES_MASK      (0xffffffff ^ BYTES_SH(7))

  Format &Format::set_8bit(void) {
    _format &= FLOAT_MASK;
    _format &= BYTES_MASK;
    _format |= BYTES_SH(1);
    return *this;
  }

  Format &Format::set_16bit(void) {
    _format &= FLOAT_MASK;
    _format &= BYTES_MASK;
    _format |= BYTES_SH(2);
    return *this;
  }

  Format &Format::set_32bit(void) {
    _format &= FLOAT_MASK;
    _format &= BYTES_MASK;
    _format |= BYTES_SH(4);
    return *this;
  }

  Format &Format::set_half(void) {
    _format &= FLOAT_MASK;
    _format |= FLOAT_SH(1);
    _format &= BYTES_MASK;
    _format |= BYTES_SH(2);
    return *this;
  }

  Format &Format::set_float(void) {
    _format &= FLOAT_MASK;
    _format |= FLOAT_SH(1);
    _format &= BYTES_MASK;
    _format |= BYTES_SH(4);
    return *this;
  }

  Format &Format::set_double(void) {
    _format &= FLOAT_MASK;
    _format |= FLOAT_SH(1);
    _format &= BYTES_MASK;
    _format |= BYTES_SH(0);
    return *this;
  }

  Format &Format::set_channel_type(unsigned char bytes, bool fp) {
    _format &= BYTES_MASK;
    _format |= BYTES_SH(bytes);
    _format &= FLOAT_MASK;
    _format |= FLOAT_SH(fp);
    return *this;
  }

  Format &Format::set_channel_type(const Format& other) {
    _format &= BYTES_MASK;
    _format |= BYTES_SH(T_BYTES(other._format));
    _format &= FLOAT_MASK;
    _format |= FLOAT_SH(T_FLOAT(other._format));
    return *this;
  }

  Format &Format::set_extra_channels(unsigned int e) {
    _format &= EXTRA_MASK;
    _format |= EXTRA_SH(e);
    return *this;
  }

  Format &Format::set_swap(bool s) {
    _format &= DOSWAP_MASK;
    _format |= DOSWAP_SH(s);
    return *this;
  }

  Format &Format::unset_swap(void) {
    _format &= DOSWAP_MASK;
    return *this;
  }

  Format &Format::set_endianswap(bool e) {
    _format &= ENDIAN16_MASK;
    _format |= ENDIAN16_SH(e);
    return *this;
  }

  Format &Format::unset_endianswap(void) {
    _format &= ENDIAN16_MASK;
    return *this;
  }

  Format &Format::set_swapfirst(bool f) {
    _format &= SWAPFIRST_MASK;
    _format |= SWAPFIRST_SH(f);
    return *this;
  }

  Format &Format::unset_swapfirst(void) {
    _format &= SWAPFIRST_MASK;
    return *this;
  }

  Format &Format::set_planar(bool p) {
    _format &= PLANAR_MASK;
    _format |= PLANAR_SH(p);
    return *this;
  }

  Format &Format::set_packed(void) {
    _format &= PLANAR_MASK;
    return *this;
  }

  Format &Format::set_vanilla(bool v) {
    _format &= FLAVOR_MASK;
    _format |= FLAVOR_SH(v);
    return *this;
  }

  Format &Format::set_chocolate(void) {
    _format &= FLAVOR_MASK;
    return *this;
  }

  Format &Format::set_colour_model(const ColourModel cm, unsigned int channels) {
    _format &= COLORSPACE_MASK;
    _format |= COLORSPACE_SH((int)cm);

    // TODO: What should be done if the supplied number of channels is wrong?
    switch (cm) {
    case ColourModel::Greyscale:
      channels = 1;
      break;

    case ColourModel::RGB:
    case ColourModel::CMY:
    case ColourModel::YCbCr:
    case ColourModel::YUV:
    case ColourModel::XYZ:
    case ColourModel::Lab:
    case ColourModel::HSV:
    case ColourModel::HLS:
    case ColourModel::Yxy:
    case ColourModel::LabV2:
      channels = 3;
      break;

    case ColourModel::CMYK:
    case ColourModel::YUVK:
      channels = 4;
      break;

    default:
      // Handle the MCH* colour models
      if ((int)cm > 14)
	channels = (int)cm - 14;
      break;
    }

    if ((channels > 0) && (channels < cmsMAXCHANNELS)) {
      _format &= CHANNELS_MASK;
      _format |= CHANNELS_SH(channels);
    }

    return *this;
  }

  Format &Format::set_premult_alpha(bool pa) {
    _premult_alpha = pa;
    return *this;
  }

  Format &Format::unset_premult_alpha() {
    _premult_alpha = false;
    return *this;
  }


  std::ostream& operator<< (std::ostream& out, Format f) {
    out << f.colour_model() << "[";
    if (f.extra_channels() > 0)
      out << "(" << f.channels() << "+" << f.extra_channels() << ")×";
    else
      out << f.channels() << "×";
    if (f.is_integer())
      out << (f.bytes_per_channel() * 8) << "b";
    else {
      if (f.is_half())
	out << "HP";
      else if (f.is_float())
	out << "SP";
      else if (f.is_double())
	out << "DP";
    }
    if (f.is_optimised())
      out << ", optimised";
    if (f.is_swappedfirst())
      out << ", swappedfirst";
    if (f.is_vanilla())
      out << ", vanilla";
    else
      out << ", chocolate";
    if (f.is_planar())
      out << ", planar";
    if (f.is_endianswapped())
      out << ", endianswapped";
    if (f.is_swapped())
      out << ", swapped";

    out << "]";

    return out;
  }




  /*
    class Transform
  */

  Transform::Transform(cmsHTRANSFORM t)
    : _transform(t)
  {
  }

  Transform::Transform(Profile::ptr input, const Format &informat,
		       Profile::ptr output, const Format &outformat,
		       Intent intent, cmsUInt32Number flags)
    : _transform(cmsCreateTransform(*input, (cmsUInt32Number)informat,
				    *output, (cmsUInt32Number)outformat,
				    (cmsUInt32Number)intent, flags))
  {
  }

  Transform::Transform(std::vector<Profile::ptr> profile,
		       const Format &informat, const Format &outformat,
		       Intent intent, cmsUInt32Number flags)
    : _transform(NULL)
  {
  }

  Transform::~Transform() {
    cmsDeleteTransform(_transform);
  }

  Transform::ptr Transform::Proofing(Profile::ptr input, const Format &informat,
				     Profile::ptr output, const Format &outformat,
				     Profile::ptr proofing,
				     Intent intent, Intent proofing_intent,
				     cmsUInt32Number flags)
  {
    return std::make_shared<Transform>(cmsCreateProofingTransform(*input, (cmsUInt32Number)informat,
								  *output, (cmsUInt32Number)outformat,
								  *proofing, (cmsUInt32Number)intent, (cmsUInt32Number)proofing_intent, flags));
  }

  Format Transform::input_format(void) const {
    return Format(cmsGetTransformInputFormat(_transform));
  }

  Format Transform::output_format(void) const {
    return Format(cmsGetTransformOutputFormat(_transform));
  }

  void Transform::change_formats(const Format &informat, const Format &outformat) {
    cmsChangeBuffersFormat(_transform, (cmsUInt32Number)informat, (cmsUInt32Number)outformat);
  }

  Profile::ptr Transform::device_link(double version, cmsUInt32Number flags) const {
    return std::make_shared<Profile>(cmsTransform2DeviceLink(_transform, version, flags));
  }

  void Transform::transform_buffer(const void* input, void* output, cmsUInt32Number size) const {
    cmsDoTransform(_transform, input, output, size);
  }



  /*
   */

  cmsIOHANDLER* OpenIOhandlerFromIStream(std::istream* is) {
    cmsIOHANDLER *ioh = (cmsIOHANDLER*)malloc(sizeof(cmsIOHANDLER));
    ioh->stream = (void*)is;
    ioh->ContextID = 0;
    ioh->UsedSpace = 0;
    {
      std::streampos current = is->tellg();
      is->seekg(0, std::ios_base::end);
      ioh->ReportedSize = is->tellg();
      is->seekg(current);
    }
    ioh->PhysicalFile[0] = 0;

    ioh->Read = istream_read;
    ioh->Seek = istream_seek;
    ioh->Close = istream_close;
    ioh->Tell = istream_tell;
    ioh->Write = istream_write;

    return ioh;
  }

  cmsIOHANDLER* OpenIOhandlerFromIFStream(fs::path filepath) {
    fs::ifstream ifs(filepath, std::ios_base::in);
    // How is OpenIOHandlerFromIStream not declared in this scope?!?
    cmsIOHANDLER *ioh = NULL; //OpenIOHandlerFromIStream(dynamic_cast<std::istream*>(&ifs));
    strncpy(ioh->PhysicalFile, filepath.generic_string().c_str(), cmsMAX_PATH);

    return ioh;
  }

  cmsUInt32Number istream_read(cmsIOHANDLER* iohandler, void *buffer, cmsUInt32Number size, cmsUInt32Number count) {
    std::istream *is = (std::istream*)iohandler->stream;
    is->read((char*)buffer, size * count);

    return is->gcount();
  }

  cmsBool istream_seek(cmsIOHANDLER* iohandler, cmsUInt32Number offset) {
    std::istream *is = (std::istream*)iohandler->stream;
    is->seekg(offset);

    return !is->fail();
  }

  cmsBool istream_close(cmsIOHANDLER* iohandler) {
    std::istream *is = (std::istream*)iohandler->stream;
    try {
      std::ifstream *ifs = static_cast<std::ifstream*>(is);
      ifs->close();
      return true;
    }
    catch (std::bad_cast& bc) {
      return false;
    }
  }

  cmsUInt32Number istream_tell(cmsIOHANDLER* iohandler) {
    std::istream *is = (std::istream*)iohandler->stream;
    return is->tellg();
  }

  cmsBool istream_write(cmsIOHANDLER* iohandler, cmsUInt32Number size, const void* Buffer) {
    // TODO: throw an exception!
    return false;
  }

  cmsUInt32Number ostream_read(cmsIOHANDLER* iohandler, void *Buffer, cmsUInt32Number size, cmsUInt32Number count) {
    // TODO: throw an exception!
    return 0;
  }

  cmsBool ostream_seek(cmsIOHANDLER* iohandler, cmsUInt32Number offset) {
    std::ostream *os = (std::ostream*)iohandler->stream;
    os->seekp(offset);

    return !os->fail();
  }

  cmsBool ostream_close(cmsIOHANDLER* iohandler) {
    std::ostream *os = (std::ostream*)iohandler->stream;
    try {
      std::ofstream *ofs = static_cast<std::ofstream*>(os);
      ofs->close();
      return true;
    }
    catch (std::bad_cast& bc) {
      return false;
    }
  }

  cmsUInt32Number ostream_tell(cmsIOHANDLER* iohandler) {
    std::ostream *os = (std::ostream*)iohandler->stream;
    return os->tellp();
  }

  cmsBool ostream_write(cmsIOHANDLER* iohandler, cmsUInt32Number size, const void* buffer) {
    std::ostream *os = (std::ostream*)iohandler->stream;
    os->write((const char*)buffer, size);
    return !os->fail();
  }

}; // namespace CMS

//! Throw a LibraryError exception whem LCMS2 returns an error
void lcms2_errorhandler(cmsContext ContextID, cmsUInt32Number ErrorCode, const char *Text) {
  throw PhotoFinish::LibraryError("LCMS2", Text);
}

void lcms2_error_adaptor(void) {
  cmsSetLogErrorHandler(lcms2_errorhandler);
}

