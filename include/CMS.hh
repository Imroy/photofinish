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
#pragma once

#include <istream>
#include <ostream>
#include <memory>
#include <boost/filesystem.hpp>
#include <lcms2.h>
#include <lcms2_plugin.h>
#include "Exception.hh"

namespace fs = boost::filesystem;

namespace CMS {

  class Transform;

  //! Wrap LCMS2's cmsHPROFILE
  class Profile {
  private:
    cmsHPROFILE _profile;

    //! Private constructor for use by named constructors
    inline Profile(cmsHPROFILE p) { _profile = p; }

    //! Private method for writing a string tag
    void write_MLU(cmsTagSignature sig, std::string language, std::string country, std::string text);
    //! Private method for writing a wide string tag
    void write_MLU(cmsTagSignature sig, std::string language, std::string country, std::wstring text);

    //! Private method for reading a string tag via the simplified cmsGetProfileInfoASCII function
    std::string read_info(cmsInfoType type, std::string language, std::string country) const;
    //! Private method for reading a string tag via the simplified cmsGetProfileInfo function
    std::wstring read_info_wide(cmsInfoType type, std::string language, std::string country) const;

    // To allow the static constructors (and anyone else?!?) to use make_shared on the private constructor
    // TODO: Since this is specific to GNU libstdc++, #ifdef's should be used to pull in each platform's version.
    friend class __gnu_cxx::new_allocator<Profile>;

  public:
    //! Empty constructor
    Profile();

    //! Copy constructor
    Profile(const Profile& other);

    //! Constructor from file path
    Profile(fs::path filepath);

    //! Constructor from memory
    Profile(const unsigned char* data, cmsUInt32Number size);

    //! Constructor from an istream
    Profile(std::istream stream);

    //! Deconstructor
    ~Profile();

    //! Cast to a profile handle for direct use with LCMS2
    inline operator cmsHPROFILE() const { return _profile; }

    //! Shared pointer typedef
    typedef std::shared_ptr<Profile> ptr;

    //! Named constructor
    static ptr Lab4(void);
      
    //! Named constructor
    static ptr sRGB(void);
      
    //! Named constructor
    static ptr sGrey(void);

    //! Set the description tag
    void set_description(std::string language, std::string country, std::string text);
    //! Set the description tag with a wide string
    void set_description(std::string language, std::string country, std::wstring text);

    //! Get the description tag
    std::string description(std::string language, std::string country) const;
    //! Get the description tag in a wide string
    std::wstring description_wide(std::string language, std::string country) const;

    //! Set the manufacturer tag
    void set_manufacturer(std::string language, std::string country, std::string text);
    //! Set the manufacturer tag with a wide string
    void set_manufacturer(std::string language, std::string country, std::wstring text);

    //! Get the manufacturer tag
    std::string manufacturer(std::string language, std::string country) const;
    //! Get the manufacturer tag in a wide string
    std::wstring manufacturer_wide(std::string language, std::string country) const;

    //! Set the model tag
    void set_model(std::string language, std::string country, std::string text);
    //! Set the model tag with a wide string
    void set_model(std::string language, std::string country, std::wstring text);

    //! Get the model tag
    std::string model(std::string language, std::string country) const;
    //! Get the model tag in a wide string
    std::wstring model_wide(std::string language, std::string country) const;

    //! Set the copyright tag
    void set_copyright(std::string language, std::string country, std::string text);
    //! Set the copyright tag with a wide string
    void set_copyright(std::string language, std::string country, std::wstring text);

    //! Get the copyright tag
    std::string copyright(std::string language, std::string country) const;
    //! Get the copyright tag in a wide string
    std::wstring copyright_wide(std::string language, std::string country) const;

    void save_to_mem(unsigned char* &dest, unsigned int &size) const;
      
  }; // class Profile

  //! An enum class of LCMS2's colour models
  enum class ColourModel {
    Any = 0,
    Greyscale = 3,
    RGB,
    CMY,
    CMYK,
    YCbCr,
    YUV,	// Lu'v'
    XYZ,
    Lab,	// Lab v4
    YUVK,	// Lu'v'K
    HSV,
    HLS,
    Yxy,

    MCH1,
    MCH2,
    MCH3,
    MCH4,
    MCH5,
    MCH6,
    MCH7,
    MCH8,
    MCH9,
    MCH10,
    MCH11,
    MCH12,
    MCH13,
    MCH14,
    MCH15,

    LabV2,	// Lab v2

  }; // enum class ColourModel

  std::ostream& operator<< (std::ostream& out, ColourModel model);

  //! Wrap LCMS2's pixel format
  class Format {
  private:
    cmsUInt32Number _format;
    bool _premult_alpha;

    //! Private constructor for use by named constructors
    Format(cmsUInt32Number f, bool pa = false);

    friend class Transform;

  public:
    //! Empty constructor
    Format();

    //! Cast to an unsigned int for direct use with LCMS2
    inline operator cmsUInt32Number() const { return _format; }

    //! Named constructor
    static Format Grey8(void);

    //! Named constructor
    static Format Grey16(void);

    //! Named constructor
    static Format RGB8(void);

    //! Named constructor
    static Format RGB16(void);

    //! Named constructor
    static Format CMYK8(void);

    //! Named constructor
    static Format LabFloat(void);

    //! Named constructor
    static Format LabDouble(void);

    //! Set to 8 bit bytes per channel
    Format &set_8bit(void);

    //! Is the format 8-bits per channel?
    inline bool is_8bit(void) const { return (T_BYTES(_format) == 1) && (T_FLOAT(_format) == 0); }

    //! Set to 16 bits per channel
    Format &set_16bit(void);

    //! Is the format 16-bits (integer) per channel?
    inline bool is_16bit(void) const { return (T_BYTES(_format) == 2) && (T_FLOAT(_format) == 0); }

    //! Set to 32 bits per channel
    Format &set_32bit(void);

    //! Is the format 32-bits (integer) per channel?
    inline bool is_32bit(void) const { return (T_BYTES(_format) == 4) && (T_FLOAT(_format) == 0); }

    //! Set to 16 bit half-precision floating point values per channel
    Format &set_half(void);

    //! Is the format a half-precision floating point value(s) per channel?
    inline bool is_half(void) const { return (T_BYTES(_format) == 2) && (T_FLOAT(_format) == 1); }

    //! Set to 32 bit single-precision floating point values per channel
    Format &set_float(void);

    //! Is the format a single-precision floating point value(s) per channel?
    inline bool is_float(void) const { return (T_BYTES(_format) == 4) && (T_FLOAT(_format) == 1); }

    //! Set to 64 bit double-precision floating point value(s) per channel
    Format &set_double(void);

    //! Is the format a double-precision floating point value(s) per channel?
    inline bool is_double(void) const { return (T_BYTES(_format) == 0) && (T_FLOAT(_format) == 1); }

    //! Set the channel type (bytes and float flag)
    Format &set_channel_type(unsigned char bytes, bool fp = false);

    //! Set the channel type (bytes and float flag) from another Format object
    Format &set_channel_type(const Format& other);

    //! Set the channel type (bytes and float flag) from the template type
    template <typename P>
    Format &set_channel_type(void);

    //! Is the format integer?
    inline bool is_integer(void) const { return (T_FLOAT(_format) == 0); }

    //! Set to an integer type
    Format &set_integer(void);

    //! Is the format floating point?
    inline bool is_fp(void) const { return (T_FLOAT(_format) == 1); }

    //! Set to a floating point type
    Format &set_fp(void);

    inline bool is_optimised(void) const { return (T_OPTIMIZED(_format) == 1); }

    //! Set the number of channels
    Format &set_channels(unsigned int c);

    //! Get the number of channels
    inline unsigned int channels(void) const { return T_CHANNELS(_format); }

    //! Set the number of 'extra' channels e.g alpha
    Format &set_extra_channels(unsigned int e);

    //! Get the number of 'extra' channels e.g alpha
    inline unsigned int extra_channels(void) const { return T_EXTRA(_format); }

    //! Get the total number of channels i.e channels() + extra_channels()
    inline unsigned int total_channels(void) const { return T_CHANNELS(_format) + T_EXTRA(_format); }

    inline unsigned int bytes_per_channel(void) const { unsigned int b = T_BYTES(_format); return b == 0 ? 8 : b; }

    inline unsigned int bytes_per_pixel(void) const { return this->bytes_per_channel() * this->total_channels(); }

    //! Set the format to be planar
    Format &set_planar(bool p = true);

    //! Set the format to be chunky
    Format &set_chunky(void);

    //! Is the format planar?
    inline bool is_planar(void) const { return T_PLANAR(_format); }

    //! Is the format chunky?
    inline bool is_chunky(void) const { return !T_PLANAR(_format); }

    Format &set_endian16_swap(bool e = true);

    Format &unset_endian16_swap(void);

    inline bool is_endian16_swapped(void) const { return T_ENDIAN16(_format); }

    //! Set the format as being swapped e.g BGR
    Format &set_swap(bool s = true);

    //! Set the format as not being swapped e.g RGB
    Format &unset_swap(void);

    //! Is the channel order swapped?
    inline bool is_swapped(void) const { return T_DOSWAP(_format); }

    Format &set_swapfirst(bool f = true);

    Format &unset_swapfirst(void);

    inline bool is_swappedfirst(void) const { return T_SWAPFIRST(_format); }

    //! Set the format to be packed
    Format &set_packed(void);

    //! Is the format packed?
    inline bool is_packed(void) const { return T_PLANAR(_format) == 0; }

    //! Set the flavour to 'vanilla' i.e minimum value is white
    Format &set_vanilla(bool v = true);

    //! Set the flavour to 'chocolate' i.e minimum value is black
    Format &set_chocolate(void);

    //! Is the flavour 'vanilla'? i.e minimum value is white
    inline bool is_vanilla(void) const { return T_FLAVOR(_format); }

    //! Is the flavour 'chocolate'? i.e minimum value is black
    inline bool is_chocolate(void) const { return T_FLAVOR(_format) == 0; }

    //! Set the colour model and number of channels
    //! 'channels' is only used if the colour model is unknown
    Format &set_colour_model(const ColourModel cm, unsigned int channels = 0);

    //! Get the colour model of the pixel format
    inline ColourModel colour_model(void) const { return (ColourModel)T_COLORSPACE(_format); }

    Format &set_premult_alpha(bool pa = true);

    Format &unset_premult_alpha();

    inline bool is_premult_alpha(void) const { return _premult_alpha; }

    //! Get the maximum value used/supported by this format
    template <typename T>
    T scaleval(void) {
      if (is_8bit())
	return 255;

      if (is_16bit())
	return 65535;

      if (is_32bit())
	return 4294967295;

      return 1;	// Assume floating point
    }

  }; // class Format

  template <>
  inline Format &Format::set_channel_type<unsigned char>(void) { return set_8bit(); }

  template <>
  inline Format &Format::set_channel_type<unsigned short int>(void) { return set_16bit(); }

  template <>
  inline Format &Format::set_channel_type<unsigned int>(void) { return set_32bit(); }

  template <>
  inline Format &Format::set_channel_type<float>(void) { return set_float(); }

  template <>
  inline Format &Format::set_channel_type<double>(void) { return set_double(); }


  std::ostream& operator<< (std::ostream& out, Format f);

  //! Wrap LCMS2's intents
  enum class Intent {
    // ICC intents
    Perceptual,
    Relative_colormetric,
    Saturation,
    Absolute_colormetric,

    // non-ICC intents
    Preserve_k_only_perceptual = 10,
    Preserve_k_only_relative_colormetric,
    Preserve_k_only_saturation,
    Preserve_k_only_absolute_colormetric,
    Preserve_k_plane_perceptual,
    Preserve_k_plane_relative_colormetric,
    Preserve_k_plane_saturation,
    Preserve_k_plane_absolute_colormetric,
  };



  //! Wrap LCMS2's transform object
  class Transform {
  private:
    cmsHTRANSFORM _transform;
    bool _one_is_planar;

    //! Private constructor
    Transform(cmsHTRANSFORM t);

    // To allow the static constructors (and anyone else?!?) to use make_shared on the private constructor
    friend class __gnu_cxx::new_allocator<Transform>;

  public:
    //! Construct a transform from two profiles and formats
    Transform(Profile::ptr input, const Format &informat,
	      Profile::ptr output, const Format &outformat,
	      Intent intent, cmsUInt32Number flags);

    //! Construct a transform from multiple profiles
    Transform(std::vector<Profile::ptr> profile,
	      const Format &informat, const Format &outformat,
	      Intent intent, cmsUInt32Number flags);

    //! Deconstructor
    ~Transform();

    typedef std::shared_ptr<Transform> ptr;

    //! Named constructor for creating a proofing transform
    static ptr Proofing(Profile::ptr input, const Format &informat,
			Profile::ptr output, const Format &outformat,
			Profile::ptr proofing,
			Intent intent, Intent proofing_intent,
			cmsUInt32Number flags);

    //! Get the input format
    Format input_format(void) const;

    //! Get the output format
    Format output_format(void) const;

    //! Change the input and output formats
    void change_formats(const Format &informat, const Format &outformat);

    bool one_is_planar(void) const { return _one_is_planar; }

    //! Create a device link profile from this transform
    Profile::ptr device_link(double version, cmsUInt32Number flags) const;

    void transform_buffer(const unsigned char* input, unsigned char* output, cmsUInt32Number size) const;

    void transform_buffer_planar(const unsigned char* input, unsigned char* output,
				 cmsUInt32Number PixelsPerLine, cmsUInt32Number LineCount,
				 cmsUInt32Number BytesPerLineIn, cmsUInt32Number BytesPerLineOut,
				 cmsUInt32Number BytesPerPlaneIn, cmsUInt32Number BytesPerPlaneOut) const;

  }; // class Transform

  // istream IO handler
  cmsIOHANDLER* OpenIOhandlerFromIStream(std::istream* is);
  cmsIOHANDLER* OpenIOhandlerFromIFStream(fs::path filepath);

  cmsUInt32Number istream_read(cmsIOHANDLER* iohandler, void* Buffer, cmsUInt32Number size, cmsUInt32Number count);
  cmsBool istream_seek(cmsIOHANDLER* iohandler, cmsUInt32Number offset);
  cmsBool istream_close(cmsIOHANDLER* iohandler);
  cmsUInt32Number istream_tell(cmsIOHANDLER* iohandler);
  cmsBool istream_write(cmsIOHANDLER* iohandler, cmsUInt32Number size, const void* Buffer);

  // ostream IO handlers
  cmsUInt32Number ostream_read(cmsIOHANDLER* iohandler, void* Buffer, cmsUInt32Number size, cmsUInt32Number count);
  cmsBool ostream_seek(cmsIOHANDLER* iohandler, cmsUInt32Number offset);
  cmsBool ostream_close(cmsIOHANDLER* iohandler);
  cmsUInt32Number ostream_tell(cmsIOHANDLER* iohandler);
  cmsBool ostream_write(cmsIOHANDLER* iohandler, cmsUInt32Number size, const void* Buffer);

}; // namespace CMS

//! Set up an error handler with LCMS2 that will throw a LibraryError exception
void lcms2_error_adaptor(void);
