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
  subst_table EXIF_key_subst = {
    StrPair("Aperture",			"Exif.Image.ApertureValue"),
    StrPair("Artist",			"Exif.Image.Artist"),
    StrPair("CameraSerialNumber",	"Exif.Image.CameraSerialNumber"),
    StrPair("Copyright",		"Exif.Image.Copyright"),
    StrPair("ExifImageLength",		"Exif.Image.ImageLength"),
    StrPair("ExifImageWidth",		"Exif.Image.ImageWidth"),
    StrPair("LightSource",		"Exif.Image.LightSource"),
    StrPair("Make",			"Exif.Image.Make"),
    StrPair("MaxApertureValue",		"Exif.Image.MaxApertureValue"),
    StrPair("Model",			"Exif.Image.Model"),
    StrPair("ResolutionUnit",		"Exif.Image.ResolutionUnit"),
    StrPair("ShutterSpeed",		"Exif.Image.ShutterSpeedValue"),
    StrPair("Software",			"Exif.Image.Software"),
    StrPair("XResolution",		"Exif.Image.XResolution"),
    StrPair("YResolution",		"Exif.Image.YResolution"),

    StrPair("ColorSpace",		"Exif.Photo.ColorSpace"),
    StrPair("DateTimeOriginal",		"Exif.Photo.DateTimeOriginal"),
    StrPair("DigitalZoomRatio",		"Exif.Photo.DigitalZoomRatio"),
    StrPair("ExifVersion",		"Exif.Photo.ExifVersion"),
    StrPair("ExifIFD:Flash",		"Exif.Photo.Flash"),
    StrPair("ExifIFD:ISO",		"Exif.Photo.ISOSpeedRatings"),
    StrPair("ExifIFD:MeteringMode",	"Exif.Photo.MeteringMode"),
    StrPair("ExposureIndex",		"Exif.Photo.ExposureIndex"),
    StrPair("ExposureMode",		"Exif.Photo.ExposureMode"),
    StrPair("ExposureProgram",		"Exif.Photo.ExposureProgram"),
    StrPair("ExposureTime",		"Exif.Photo.ExposureTime"),
    StrPair("Flash",			"Exif.Photo.Flash"),
    StrPair("FNumber",			"Exif.Photo.FNumber"),
    StrPair("FocalLength",		"Exif.Photo.FocalLength"),
    StrPair("FocalLength35efl",		"Exif.Photo.FocalLengthIn35mmFilm"),
    StrPair("FocalLengthIn35mmFormat",	"Exif.Photo.FocalLengthIn35mmFilm"),
    StrPair("ISO",			"Exif.Photo.ISOSpeedRatings"),
    StrPair("LensMake",			"Exif.Photo.LensMake"),
    StrPair("LensModel",		"Exif.Photo.LensModel"),
    StrPair("LensSerialNumber",		"Exif.Photo.LensSerialNumber"),
    StrPair("MeteringMode",		"Exif.Photo.MeteringMode"),
    StrPair("SceneType",		"Exif.Photo.SceneType"),
    StrPair("UserComment",		"Exif.Photo.UserComment"),

    StrPair("GPSAltitude",		"Exif.GPSInfo.GPSAltitude"),
    StrPair("GPSAltitudeRef",		"Exif.GPSInfo.GPSAltitudeRef"),
    StrPair("GPSDateStamp",		"Exif.GPSInfo.GPSDateStamp"),
    StrPair("GPSDifferential",		"Exif.GPSInfo.GPSDifferential"),
    StrPair("GPSDOP",			"Exif.GPSInfo.GPSDOP"),
    StrPair("GPSLatitude",		"Exif.GPSInfo.GPSLatitude"),
    StrPair("GPSLatitudeRef",		"Exif.GPSInfo.GPSLatitudeRef"),
    StrPair("GPSLongitude",		"Exif.GPSInfo.GPSLongitude"),
    StrPair("GPSLongitudeRef",		"Exif.GPSInfo.GPSLongitudeRef"),
    StrPair("GPSMeasureMode",		"Exif.GPSInfo.GPSMeasureMode"),
    StrPair("GPSTimeStamp",		"Exif.GPSInfo.GPSTimeStamp"),
  };

  Exiv2::ExifKey exif_key_read(std::string key_string) {
    for (auto i : EXIF_key_subst)
      if (boost::iequals(key_string, i.first)) {
	key_string = i.second;
	break;
      }

    return Exiv2::ExifKey(key_string);
  }

#define Key(k, h) std::make_pair<std::string, subst_table>(k, h)

  std::map<std::string, subst_table> EXIF_value_subst = {
    Key("Exif.GPSInfo.GPSAltitudeRef", subst_table({
	    StrPair("Above sea level",		"0"),
	      StrPair("Below sea level",	"1"),
	      })),

    Key("Exif.GPSInfo.GPSDestDistanceRef", subst_table({
	    StrPair("kilometres",		"75"),
	      StrPair("kilometers",		"75"),
	      StrPair("km",			"75"),
	      StrPair("miles",			"77"),
	      StrPair("mi",			"77"),
	      StrPair("nautical miles",		"78"),
	      })),

    Key("Exif.GPSInfo.GPSDifferential", subst_table({
	    StrPair("No Correction",		"0"),
	      StrPair("Differential Corrected",	"1"),
	      })),

    Key("Exif.GPSInfo.GPSLatitudeRef", subst_table({
	    StrPair("North",	"78"),
	      StrPair("South",	"83"),
	      })),

    Key("Exif.GPSInfo.GPSLongitudeRef", subst_table({
	    StrPair("East",	"69"),
	      StrPair("West",	"87"),
	      })),

    Key("Exif.GPSInfo.GPSMeasureMode", subst_table({
	    StrPair("2-Dimensional Measurement",	"2"),
	      StrPair("Two-Dimensional Measurement",	"2"),
	      StrPair("3-Dimensional Measurement",	"3"),
	      StrPair("Three-Dimensional Measurement",	"3"),
	      })),

    Key("Exif.GPSInfo.GPSSpeedRef", subst_table({
	    StrPair("km/h",	"75"),
	      StrPair("mph",	"77"),
	      StrPair("knots",	"78"),
	      })),

    Key("Exif.GPSInfo.GPSStatus", subst_table({
	    StrPair("Measurement Active",		"65"),
	      StrPair("Measurement in progress",	"65"),
	      StrPair("Measurement Void",		"86"),
	      StrPair("Measurement Interoperability",	"86"),
	      })),

    Key("Exif.GPSInfo.GPSTrackRef", subst_table({
	    StrPair("Magnetic north",	"77"),
	      StrPair("True north",	"84"),
	      })),

    Key("Exif.Photo.ExposureProgram", subst_table({
	    StrPair("Not Defined",			"0"),
	      StrPair("Manual",				"1"),
	      StrPair("Program AE",			"2"),
	      StrPair("Auto",				"2"),
	      StrPair("Aperture-priority AE",		"3"),
	      StrPair("Aperture priority",		"3"),
	      StrPair("Shutter speed priority AE",	"4"),
	      StrPair("Shutter priority",		"4"),
	      StrPair("Creative (Slow speed)",		"5"),
	      StrPair("Creative program",		"5"),
	      StrPair("Action (High speed)",		"6"),
	      StrPair("Action program",			"6"),
	      StrPair("Portrait",			"7"),
	      StrPair("Portrait mode",			"7"),
	      StrPair("Landscape",			"8"),
	      StrPair("Landscape mode",			"8"),
	      StrPair("Bulb",				"9"), // not standard EXIF, but is used by the Canon EOS 7D
	      })),

    Key("Exif.Photo.Flash", subst_table({
	    StrPair("No Flash",							"0"),
	      StrPair("Fired",							"1"),
	      StrPair("Fired, Return not detected",				"5"),
	      StrPair("Fired, Return light not detected",			"5"),
	      StrPair("Fired, Return detected",					"7"),
	      StrPair("Fired, Return light detected",				"7"),
	      StrPair("On, Did not fire",					"8"),
	      StrPair("Yes, Did not fire",					"8"),
	      StrPair("On, Fired",						"9"),
	      StrPair("Yes, compulsory",					"9"),
	      StrPair("On, Return not detected",				"13"),
	      StrPair("Yes, Return light not detected",				"13"),
	      StrPair("On, Return detected",					"15"),
	      StrPair("Yes, Return light detected",				"15"),
	      StrPair("Off, Did not fire",					"16"),
	      StrPair("No, compulsory",						"16"),
	      StrPair("Off, Did not fire, Return not detected",			"20"),
	      StrPair("No, did not fire, return light not detected",		"20"),
	      StrPair("Auto, Did not fire",					"24"),
	      StrPair("No, auto",						"24"),
	      StrPair("Auto, Fired",						"25"),
	      StrPair("Yes, auto",						"25"),
	      StrPair("Auto, Fired, Return not detected",			"29"),
	      StrPair("Auto, Fired, Return detected",				"31"),
	      StrPair("No flash function",					"32"),
	      StrPair("Off, No flash function",					"48"),
	      StrPair("Fired, Red-eye reduction",				"65"),
	      StrPair("Fired, Red-eye reduction, Return not detected",		"69"),
	      StrPair("Fired, Red-eye reduction, Return detected",		"71"),
	      StrPair("On, Red-eye reduction",					"72"),
	      StrPair("On, Red-eye reduction, Return not detected",		"77"),
	      StrPair("On, Red-eye reduction, Return detected",			"79"),
	      StrPair("Off, Red-eye reduction",					"80"),
	      StrPair("Auto, Did not fire, Red-eye reduction",			"88"),
	      StrPair("Auto, Fired, Red-eye reduction",				"89"),
	      StrPair("Auto, Fired, Red-eye reduction, Return not detected",	"93"),
	      StrPair("Auto, Fired, Red-eye reduction, Return detected",	"95"),
	      })),

    Key("Exif.Image.LightSource", subst_table({
	    StrPair("Unknown",						"0"),
	      StrPair("Daylight",					"1"),
	      StrPair("Fluorescent",					"2"),
	      StrPair("Tungsten (Incandescent)",			"3"),
	      StrPair("Tungsten (incandescent light)",			"3"),
	      StrPair("Flash",						"4"),
	      StrPair("Fine Weather",					"9"),
	      StrPair("Cloudy",						"10"),
	      StrPair("Cloudy weather",					"10"),
	      StrPair("Shade",						"11"),
	      StrPair("Daylight Fluorescent",				"12"), // (D 5700 - 7100K)
	      StrPair("Daylight fluorescent (D 5700 - 7100K)",		"12"),
	      StrPair("Day White Fluorescent",				"13"), // (N 4600 - 5500K)
	      StrPair("Day white fluorescent (N 4600 - 5400K)",		"13"),
	      StrPair("Cool White Fluorescent",				"14"), // (W 3800 - 4500K)
	      StrPair("Cool white fluorescent (W 3900 - 4500K)",	"14"),
	      StrPair("White Fluorescent",				"15"), // (WW 3250 - 3800K)
	      StrPair("White fluorescent (WW 3200 - 3700K)",		"15"),
	      StrPair("Warm White Fluorescent",				"16"), // (L 2600 - 3250K)
	      StrPair("Standard Light A",				"17"),
	      StrPair("Standard Light B",				"18"),
	      StrPair("Standard Light C",				"19"),
	      StrPair("D55",						"20"),
	      StrPair("D65",						"21"),
	      StrPair("D75",						"22"),
	      StrPair("D50",						"23"),
	      StrPair("ISO Studio Tungsten",				"24"),
	      StrPair("Other",						"255"),
	      })),

    Key("Exif.Photo.MeteringMode", subst_table({
	    StrPair("Unknown",				"0"),
	      StrPair("Average",			"1"),
	      StrPair("Center weighted average",	"2"),
	      StrPair("Center-weighted average",	"2"),
	      StrPair("Spot",				"3"),
	      StrPair("Multi-spot",			"4"),
	      StrPair("Multi-segment",			"5"),
	      StrPair("Partial",			"6"),
	      StrPair("Other",				"255"),
	      })),

    Key("Exif.Image.ResolutionUnit", subst_table({
	    StrPair("None",		"1"),
	      StrPair("inches",		"2"),
	      StrPair("inch",		"2"),
	      StrPair("cm",		"3"),
	      StrPair("centimetre",	"3"),
	      StrPair("centimeter",	"3"),
	      })),

    Key("Exif.Photo.SceneType", subst_table({
	    StrPair("Directly Photographed", "1"),
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
  Exiv2::Value::AutoPtr exif_value_read(Exiv2::ExifKey key, std::string value_string) {
    Exiv2::TypeId type = key.defaultTypeId();
    if (type == Exiv2::unsignedRational)
      return Exiv2::Value::AutoPtr(new Exiv2::URationalValue(parse_URational(value_string)));
    if (type == Exiv2::signedRational)
      return Exiv2::Value::AutoPtr(new Exiv2::RationalValue(parse_Rational(value_string)));

    EXIF_value_subst["Exif.GPSInfo.GPSImgDirectionRef"] = EXIF_value_subst["Exif.GPSInfo.GPSTrackRef"];
    EXIF_value_subst["Exif.GPSInfo.GPSDestBearingRef"] = EXIF_value_subst["Exif.GPSInfo.GPSTrackRef"];
    EXIF_value_subst["Exif.GPSInfo.GPSDestLatitudeRef"] = EXIF_value_subst["Exif.GPSInfo.GPSLatitudeRef"];
    EXIF_value_subst["Exif.GPSInfo.GPSDestLongitudeRef"] = EXIF_value_subst["Exif.GPSInfo.GPSLongitudeRef"];

    auto hi = EXIF_value_subst.find(key.key());
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


