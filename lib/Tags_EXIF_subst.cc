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
#include <string>
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "Tags.hh"

namespace PhotoFinish {

  //! Map from Image::Exiftool tag names to Exiv2's tag names
  void populate_EXIF_subst(hash& table) {

    table["Aperture"]			= "Exif.Image.ApertureValue";
    table["Artist"]			= "Exif.Image.Artist";
    table["BWMode"]			= "";
    table["CameraID"]			= "";
    table["CameraSerialNumber"]		= "Exif.Image.CameraSerialNumber";
    table["CameraType"]			= "";
    table["CircleOfConfusion"]		= "";
    table["ColorSpace"]			= "Exif.Photo.ColorSpace";
    table["ComponentsConfiguration"]	= "";
    table["CompressedBitsPerPixel"]	= "";
    table["Compression"]		= "";
    table["Contrast"]			= "";
    table["Copyright"]			= "Exif.Image.Copyright";
    table["CreateDate"]			= "";
    table["CustomRendered"]		= "";
    table["DataDump"]			= "";
    table["DigitalZoom"]		= "";
    table["ExifIFD:Lens"]		= "";	// ???
    table["ExifImageLength"]		= "Exif.Image.ImageLength";
    table["ExifImageWidth"]		= "Exif.Image.ImageWidth";
    table["ExifVersion"]		= "Exif.Photo.ExifVersion";
    table["ExposureCompensation"]	= "";
    table["FileSource"]			= "";
    table["FlashpixVersion"]		= "";
    table["FocalPlaneDiagonal"]		= "";
    table["GainControl"]		= "";
    table["HyperfocalDistance"]		= "";
    table["ImageDescription"]		= "";
    table["ImageHeight"]		= "";
    table["ImageSize"]			= "";
    table["ImageWidth"]			= "";
    table["InteropIndex"]		= "";
    table["InteropVersion"]		= "";
    table["LensDistortionParams"]	= "";
    table["LightSource"]		= "";
    table["LightValue"]			= "";
    table["Macro"]			= "";
    table["Make"]			= "Exif.Image.Make";
    table["MaxApertureValue"]		= "Exif.Image.MaxApertureValue";
    table["Model"]			= "Exif.Image.Model";
    table["ModifyDate"]			= "";
    table["OneTouchWB"]			= "";
    table["Orientation"]		= "";
    table["PreCaptureFrames"]		= "";
    table["Quality"]			= "";
    table["Resolution"]			= "";
    table["ResolutionUnit"]		= "Exif.Image.ResolutionUnit";
    table["Saturation"]			= "";
    table["ScaleFactor35efl"]		= "";
    table["SceneCaptureType"]		= "";
    table["Sharpness"]			= "";
    table["ShutterSpeed"]		= "Exif.Image.ShutterSpeedValue";
    table["Software"]			= "Exif.Image.Software";
    table["SpecialMode"]		= "";
    table["TagsFromFile"]		= "";
    table["ThumbnailLength"]		= "";
    table["ThumbnailOffset"]		= "";
    table["UserComment"]		= "Exif.Photo.UserComment";
    table["WhiteBalance"]		= "";
    table["XResolution"]		= "Exif.Image.XResolution";
    table["YCbCrPositioning"]		= "";
    table["YResolution"]		= "Exif.Image.YResolution";

    table["DateTimeOriginal"]		= "Exif.Photo.DateTimeOriginal";
    table["DigitalZoomRatio"]		= "Exif.Photo.DigitalZoomRatio";
    table["ExifIFD:Flash"]		= "Exif.Photo.Flash";
    table["ExifIFD:ISO"]		= "Exif.Photo.ISOSpeedRatings";
    table["ExifIFD:MeteringMode"]	= "Exif.Photo.MeteringMode";
    table["ExposureIndex"]		= "Exif.Photo.ExposureIndex";
    table["ExposureMode"]		= "Exif.Photo.ExposureMode";
    table["ExposureProgram"]		= "Exif.Photo.ExposureProgram";
    table["ExposureTime"]		= "Exif.Photo.ExposureTime";
    table["Flash"]			= "Exif.Photo.Flash";
    table["FNumber"]			= "Exif.Photo.FNumber";
    table["FocalLength"]		= "Exif.Photo.FocalLength";
    table["FocalLength35efl"]		= "Exif.Photo.FocalLengthIn35mmFilm";
    table["FocalLengthIn35mmFormat"]	= "Exif.Photo.FocalLengthIn35mmFilm";
    table["ISO"]			= "Exif.Photo.ISOSpeedRatings";
    table["LensMake"]			= "Exif.Photo.LensMake";
    table["LensModel"]			= "Exif.Photo.LensModel";
    table["LensSerialNumber"]		= "Exif.Photo.LensSerialNumber";
    table["MeteringMode"]		= "Exif.Photo.MeteringMode";
    table["SceneType"]			= "Exif.Photo.SceneType";

    table["GPSAltitude"]		= "Exif.GPSInfo.GPSAltitude";
    table["GPSAltitudeRef"]		= "Exif.GPSInfo.GPSAltitudeRef";
    table["GPSDateStamp"]		= "Exif.GPSInfo.GPSDateStamp";
    table["GPSDifferential"]		= "Exif.GPSInfo.GPSDifferential";
    table["GPSDOP"]			= "Exif.GPSInfo.GPSDOP";
    table["GPSLatitude"]		= "Exif.GPSInfo.GPSLatitude";
    table["GPSLatitudeRef"]		= "Exif.GPSInfo.GPSLatitudeRef";
    table["GPSLongitude"]		= "Exif.GPSInfo.GPSLongitude";
    table["GPSLongitudeRef"]		= "Exif.GPSInfo.GPSLongitudeRef";
    table["GPSMeasureMode"]		= "Exif.GPSInfo.GPSMeasureMode";
    table["GPSTimeStamp"]		= "Exif.GPSInfo.GPSTimeStamp";
  }

#define Key(k, h) std::make_pair<std::string, hash>(k, h)
#define HashPair(s, v) std::make_pair<std::string, std::string>(s, v)

  std::map<std::string, hash> EXIF_value_subst = {
    Key("Exif.GPSInfo.GPSAltitudeRef", hash({
	    HashPair("Above sea level", "0"),
	      HashPair("Below sea level", "1"),
	      })),

    Key("Exif.GPSInfo.GPSDifferential", hash({
	    HashPair("No Correction", "0"),
	      HashPair("Differential Corrected", "1"),
	      })),

    Key("Exif.GPSInfo.GPSLatitudeRef", hash({
	    HashPair("North", "78"),
	      HashPair("South", "83"),
	      })),

    Key("Exif.GPSInfo.GPSLongitudeRef", hash({
	    HashPair("East", "69"),
	      HashPair("West", "87"),
	      })),

    Key("Exif.GPSInfo.GPSMeasureMode", hash({
	    HashPair("2-Dimensional Measurement", "2"),
	      HashPair("Two-Dimensional Measurement", "2"),
	      HashPair("3-Dimensional Measurement", "3"),
	      HashPair("Three-Dimensional Measurement", "3"),
	      })),

    Key("Exif.Photo.ExposureProgram", hash({
	    HashPair("Not Defined", "0"),
	      HashPair("Manual", "1"),
	      HashPair("Program AE", "2"),
	      HashPair("Auto", "2"),
	      HashPair("Aperture-priority AE", "3"),
	      HashPair("Aperture priority", "3"),
	      HashPair("Shutter speed priority AE", "4"),
	      HashPair("Shutter priority", "4"),
	      HashPair("Creative (Slow speed)", "5"),
	      HashPair("Creative program", "5"),
	      HashPair("Action (High speed)", "6"),
	      HashPair("Action program", "6"),
	      HashPair("Portrait", "7"),
	      HashPair("Portrait mode", "7"),
	      HashPair("Landscape", "8"),
	      HashPair("Landscape mode", "8"),
	      HashPair("Bulb", "9"), // not standard EXIF, but is used by the Canon EOS 7D
	      })),

    Key("Exif.Photo.Flash", hash({
	    HashPair("No Flash", "0"),
	      HashPair("Fired", "1"),
	      HashPair("Fired, Return not detected", "5"),
	      HashPair("Fired, Return light not detected", "5"),
	      HashPair("Fired, Return detected", "7"),
	      HashPair("Fired, Return light detected", "7"),
	      HashPair("On, Did not fire", "8"),
	      HashPair("Yes, Did not fire", "8"),
	      HashPair("On, Fired", "9"),
	      HashPair("Yes, compulsory", "9"),
	      HashPair("On, Return not detected", "13"),
	      HashPair("Yes, Return light not detected", "13"),
	      HashPair("On, Return detected", "15"),
	      HashPair("Yes, Return light detected", "15"),
	      HashPair("Off, Did not fire", "16"),
	      HashPair("No, compulsory", "16"),
	      HashPair("Off, Did not fire, Return not detected", "20"),
	      HashPair("No, did not fire, return light not detected", "20"),
	      HashPair("Auto, Did not fire", "24"),
	      HashPair("No, auto", "24"),
	      HashPair("Auto, Fired", "25"),
	      HashPair("Yes, auto", "25"),
	      HashPair("Auto, Fired, Return not detected", "29"),
	      HashPair("Auto, Fired, Return detected", "31"),
	      HashPair("No flash function", "32"),
	      HashPair("Off, No flash function", "48"),
	      HashPair("Fired, Red-eye reduction", "65"),
	      HashPair("Fired, Red-eye reduction, Return not detected", "69"),
	      HashPair("Fired, Red-eye reduction, Return detected", "71"),
	      HashPair("On, Red-eye reduction", "72"),
	      HashPair("On, Red-eye reduction, Return not detected", "77"),
	      HashPair("On, Red-eye reduction, Return detected", "79"),
	      HashPair("Off, Red-eye reduction", "80"),
	      HashPair("Auto, Did not fire, Red-eye reduction", "88"),
	      HashPair("Auto, Fired, Red-eye reduction", "89"),
	      HashPair("Auto, Fired, Red-eye reduction, Return not detected", "93"),
	      HashPair("Auto, Fired, Red-eye reduction, Return detected", "95"),
	      })),

    Key("Exif.Photo.MeteringMode", hash({
	    HashPair("Unknown", "0"),
	      HashPair("Average", "1"),
	      HashPair("Center weighted average", "2"),
	      HashPair("Center-weighted average", "2"),
	      HashPair("Spot", "3"),
	      HashPair("Multi-spot", "4"),
	      HashPair("Multi-segment", "5"),
	      HashPair("Partial", "6"),
	      HashPair("Other", "255"),
	      })),

    Key("Exif.Image.ResolutionUnit", hash({
	    HashPair("None", "1"),
	      HashPair("inches", "2"),
	      HashPair("inch", "2"),
	      HashPair("cm", "3"),
	      HashPair("centimetre", "3"),
	      HashPair("centimeter", "3"),
	      })),

    Key("Exif.Photo.SceneType", hash({
	    HashPair("Directly Photographed", "1"),
	      })),
  };

  //! Find a close unsigned rational fraction given a floating-point value
  Exiv2::URational closest_URational(double value) {
    double margin = fabs(value) * 1e-6;
    unsigned int num = 0, den;
    for (den = 1; den < INT_MAX; den++) {
      double numf = value * den;
      if ((numf < 0) || (numf > UINT_MAX))
	break;

      num = round(numf);
      double error = fabs(num - numf);
      if (error < margin * den)
	break;
    }
    return Exiv2::URational(num, den);
  }

  //! Parse a string into an unsigned rational fraction
  Exiv2::URational parse_URational(std::string s) {
    size_t slash = s.find_first_of('/');
    if (slash == std::string::npos)
      return closest_URational(boost::lexical_cast<double>(s));

    unsigned int num, den;
    num = boost::lexical_cast<unsigned int>(s.substr(0, slash));
    den = boost::lexical_cast<unsigned int>(s.substr(slash + 1, s.length() - slash - 1));

    return Exiv2::URational(num, den);
  }

  //! Find a close rational fraction given a floating-point value
  Exiv2::Rational closest_Rational(double value) {
    double margin = fabs(value) * 1e-6;
    signed int num = 0;
    unsigned int den;
    for (den = 1; den < INT_MAX; den++) {
      double numf = value * den;
      if ((numf < INT_MIN) || (numf > INT_MAX))
	break;

      num = round(numf);
      double error = fabs(num - numf);
      if (error < margin * den)
	break;
    }
    return Exiv2::Rational(num, den);
  }

  //! Parse a string into a rational fraction
  Exiv2::Rational parse_Rational(std::string s) {
    size_t slash = s.find_first_of('/');
    if (slash == std::string::npos)
      return closest_Rational(boost::lexical_cast<double>(s));

    signed int num;
    unsigned int den;
    num = boost::lexical_cast<signed int>(s.substr(0, slash));
    den = boost::lexical_cast<unsigned int>(s.substr(slash + 1, s.length() - slash - 1));

    return Exiv2::Rational(num, den);
  }

  //! Read an EXIF value from a string, with optional substitution for enum-style values
  Exiv2::Value::AutoPtr exif_read(std::string key, Exiv2::TypeId type, std::string value_string) {
    if (type == Exiv2::unsignedRational)
      return Exiv2::Value::AutoPtr(new Exiv2::URationalValue(parse_URational(value_string)));
    if (type == Exiv2::signedRational)
      return Exiv2::Value::AutoPtr(new Exiv2::RationalValue(parse_Rational(value_string)));

    auto hi = EXIF_value_subst.find(key);
    if (hi != EXIF_value_subst.end()) {
      for (auto i : hi->second)
	if (boost::iequals(value_string, i.first)) {
	  value_string = i.second;
	  break;
	}
    }

    Exiv2::Value::AutoPtr value = Exiv2::Value::create(type);
    value->read(value_string);

    return value;
  }

}


