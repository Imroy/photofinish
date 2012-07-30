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
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <exiv2/exiv2.hpp>
#include <boost/lexical_cast.hpp>
#include <math.h>
#include "Image.hh"
#include "ImageFile.hh"
#include "Tags.hh"
#include "Exception.hh"

namespace PhotoFinish {

  Tags::Tags() {
  }

  Tags::Tags(fs::path filepath) {
    load(filepath);
  }

  void populate_EXIF_subst(hash& table);
  void populate_IPTC_subst(hash& table);
  void populate_XMP_subst(hash& table);

  Exiv2::URational parse_URational(double value) {
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

  Exiv2::URational parse_URational(std::string s) {
    size_t slash = s.find_first_of('/');
    if (slash == std::string::npos)
      return parse_URational(boost::lexical_cast<double>(s));

    unsigned int num, den;
    num = boost::lexical_cast<unsigned int>(s.substr(0, slash));
    den = boost::lexical_cast<unsigned int>(s.substr(slash + 1, s.length() - slash - 1));

    return Exiv2::URational(num, den);
  }

  Exiv2::Rational parse_Rational(double value) {
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

  Exiv2::Rational parse_Rational(std::string s) {
    size_t slash = s.find_first_of('/');
    if (slash == std::string::npos)
      return parse_Rational(boost::lexical_cast<double>(s));

    signed int num;
    unsigned int den;
    num = boost::lexical_cast<signed int>(s.substr(0, slash));
    den = boost::lexical_cast<unsigned int>(s.substr(slash + 1, s.length() - slash - 1));

    return Exiv2::Rational(num, den);
  }

  void Tags::load(fs::path filepath) {
    std::cerr << "Loading \"" << filepath.native() << "\"..." << std::endl;
    std::ifstream fin(filepath.native());
    if (fin.fail())
      throw FileOpenError(filepath.native());

    hash EXIF_subst, IPTC_subst, XMP_subst;
    populate_EXIF_subst(EXIF_subst);
    populate_IPTC_subst(IPTC_subst);
    populate_XMP_subst(XMP_subst);

    while (!fin.eof()) {
      std::string line;
      getline(fin, line);
      if (fin.eof())
	break;

      // Remove comment text
      {
	size_t x;
	if ((x = line.find_first_of('#', 1)) != std::string::npos)
	  line = line.substr(0, x - 1);
      }

      if (line.substr(0, 2) == "#@") {
	int start = line.find_first_not_of(" \t", 2);
	int end = line.find_last_not_of(" \t");
	load(".tags" / fs::path(line.substr(start, end + 1 - start)));
	continue;
      }

      if (line.substr(0, 2) == "#$") {
	int start = line.find_first_not_of(" \t", 2);
	int eq = line.find_first_of('=', start);
	int end = line.find_last_not_of(" \t");
	std::string key = line.substr(start, eq - start);
	std::string value = line.substr(eq + 1, end - eq);
	std::cerr << "\tVariable \"" << key << "\" = \"" << value << "\"" << std::endl;
	multihash::iterator vi;
	if ((vi = _variables.find(key)) != _variables.end())
	  vi->second.push_back(value);
	else {
	  std::vector<std::string> list;
	  list.push_back(value);
	  _variables[key] = list;
	}
	continue;
      }

      // Any other line beginning with '#' is a comment
      if (line[0] == '#')
	continue;

      // Lines starting with '-' are EXIF, IPTC, or XMP tags
      if (line.substr(0, 1) == "-") {
	int start = line.find_first_not_of(" \t", 1);
	int eq = line.find_first_of('=', start);
	int end = line.find_last_not_of(" \t");
	std::string value_string = line.substr(eq + 1, end - eq);

	if (line.substr(start, 3) == "XMP") {
	  std::string key_string = line.substr(start, eq - start);
	  hash::iterator si;
	  if (((si = XMP_subst.find(key_string)) != XMP_subst.end()) && (si->second.length() > 0))
	    key_string = si->second;

	  try {
	    Exiv2::XmpKey key(key_string);
	    Exiv2::Value::AutoPtr value = Exiv2::Value::create(Exiv2::xmpText);
	    value->read(value_string);
	    std::cerr << "\tXMP \"" << key << "\" = \"" << *value << "\"" << std::endl;
	    _XMPtags.add(key, value.get());
	  } catch (Exiv2::Error& e) {
	    std::cerr << "** XMP key \"" << key_string << "\" not accepted **" << std::endl;
	  }
	} else 	if (line.substr(start, 4) == "IPTC") {
	  std::string key_string = line.substr(start, eq - start);
	  hash::iterator si;
	  if (((si = IPTC_subst.find(key_string)) != IPTC_subst.end()) && (si->second.length() > 0))
	    key_string = si->second;

	  try {
	    Exiv2::IptcKey key(key_string);
	    Exiv2::Value::AutoPtr value = Exiv2::Value::create(Exiv2::asciiString);
	    value->read(value_string);
	    std::cerr << "\tIPTC \"" << key << "\" = \"" << *value << "\"" << std::endl;
	    _IPTCtags.add(key, value.get());
	  } catch (Exiv2::Error& e) {
	    std::cerr << "** IPTC key \"" << key_string << "\" not accepted **" << std::endl;
	  }
	} else {
	  std::string key_string = line.substr(start, eq - start);
	  hash::iterator si;
	  if (((si = EXIF_subst.find(key_string)) != EXIF_subst.end()) && (si->second.length() > 0))
	    key_string = si->second;

	  try {
	    Exiv2::ExifKey key(key_string);

	    Exiv2::TypeId type = key.defaultTypeId();
	    Exiv2::Value::AutoPtr value = Exiv2::Value::create(type);
	    if (type == Exiv2::unsignedRational)
	      value = Exiv2::Value::AutoPtr(new Exiv2::URationalValue(parse_URational(value_string)));
	    else if (type == Exiv2::signedRational)
	      value = Exiv2::Value::AutoPtr(new Exiv2::RationalValue(parse_Rational(value_string)));
	    else
	      value->read(value_string);

	    std::cerr << "\tEXIF \"" << key << "\" (" << type << ") = \"" << *value << "\"" << std::endl;
	    _EXIFtags.add(key, value.get());
	  } catch (Exiv2::Error& e) {
	    std::cerr << "** EXIF key \"" << key_string << "\" not accepted **" << std::endl;
	  }
	}
	continue;
      }

      std::cerr << "** Unrecognised line \"" << line << "\" in file \"" << filepath.native() << "\" **" << std::endl;
    }
    fin.close();
    std::cerr << "Finished loading \"" << filepath.native() << "\"" << std::endl;
  }

  void Tags::make_thumbnail(Image::ptr img, const D_thumbnail& dt) {
    double width, height;

    if (dt.maxwidth() * img->height() < dt.maxheight() * img->width()) {
      width = dt.maxwidth();
      height = dt.maxwidth() * img->height() / img->width();
    } else {
      width = dt.maxheight() * img->width() / img->height();
      height = dt.maxheight();
    }

    std::cerr << "Making EXIF thumbnail..." << std::endl;

    Frame frame(width, height, 0, 0, img->width(), img->height());
    Image::ptr thumbimage = frame.crop_resize(img, D_resize::lanczos(3.0));

    Destination dest;
    dest.set_jpeg(D_JPEG(50, 1, 1, false));

    JPEGFile thumbfile("");
    std::ostringstream oss;
    thumbfile.write(oss, thumbimage, dest);

    Exiv2::ExifThumb EXIFthumb(_EXIFtags);
    EXIFthumb.setJpegThumbnail((unsigned char*)oss.str().data(), oss.str().length());

    std::cerr << "Done." << std::endl;
  }

  void Tags::add_resolution(Image::ptr img) {
    Exiv2::URationalValue v;
    if (img->xres().defined()) {
      v = Exiv2::URationalValue(parse_URational(img->xres()));
      std::cerr << "\tSetting X resolution to " << v.value_[0].first << " ÷ " << v.value_[0].second << " (" << img->xres() << ") ppi." << std::endl;
      try {
	_EXIFtags["Exif.Image.XResolution"] = v;
      } catch (Exiv2::Error& e) {
	std::cerr << "** EXIF key \"Exif.Image.XResolution\" not accepted **" << std::endl;
      }
    }
    if (img->yres().defined()) {
      v = Exiv2::URationalValue(parse_URational(img->yres()));
      std::cerr << "\tSetting Y resolution to " << v.value_[0].first << " ÷ " << v.value_[0].second << " (" << img->yres() << ") ppi." << std::endl;
      try {
	_EXIFtags["Exif.Image.YResolution"] = v;
      } catch (Exiv2::Error& e) {
	std::cerr << "** EXIF key \"Exif.Image.YResolution\" not accepted **" << std::endl;
      }
    }
    _EXIFtags["Exif.Image.ResolutionUnit"] = 2;	// Inches (yuck)
  }

  void Tags::embed(fs::path filepath) const {
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filepath.native());
    assert(image.get() != 0);

    image->setExifData(_EXIFtags);
    image->setIptcData(_IPTCtags);
    image->setXmpData(_XMPtags);
    image->writeMetadata();
  }

}
