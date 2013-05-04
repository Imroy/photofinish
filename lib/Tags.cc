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
#include <math.h>
#include "Image.hh"
#include "ImageFile.hh"
#include "Tags.hh"
#include "Exception.hh"

namespace PhotoFinish {

  Tags::Tags() {
  }

  Tags::Tags(const Tags &other) :
    _searchpaths(other._searchpaths),
    _variables(other._variables),
    _EXIFtags(other._EXIFtags),
    _IPTCtags(other._IPTCtags),
    _XMPtags(other._XMPtags)
  {}

  Tags::Tags(fs::path filepath) {
    load(filepath);
  }

  Tags::ptr Tags::dupe(void) const {
    return std::make_shared<Tags>(*this);
  }

  Exiv2::ExifKey exif_key_read(std::string key_string);
  Exiv2::Value::AutoPtr exif_value_read(Exiv2::ExifKey key, std::string value_string);
  Exiv2::IptcKey iptc_key_read(std::string key_string);
  Exiv2::XmpKey xmp_key_read(std::string key_string);

  bool Tags::try_load(fs::path filepath) {
    for (auto pi : _searchpaths) {
      fs::path fullpath = pi / filepath;
      if (fs::exists(fullpath)) {
	load(fullpath);
	return true;
      }
    }
    return false;
  }

  void Tags::load(fs::path filepath) {
    std::cerr << "Loading \"" << filepath.native() << "\"..." << std::endl;
    std::ifstream fin(filepath.native());
    if (fin.fail())
      throw FileOpenError(filepath.native());

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
	fs::path filepath = line.substr(start, end + 1 - start);
	if (!try_load(filepath))
	  std::cerr << "** Could not find a tag file named \"" << filepath << "\" in the search paths **" << std::endl;
	continue;
      }

      if (line.substr(0, 2) == "#$") {
	int start = line.find_first_not_of(" \t", 2);
	int eq = line.find_first_of('=', start);
	int end = line.find_last_not_of(" \t");
	std::string key = line.substr(start, eq - start);
	std::string value = line.substr(eq + 1, end - eq);
	std::cerr << "\tVariable \"" << key << "\" = \"" << value << "\"" << std::endl;
	auto vi = _variables.find(key);
	if (vi != _variables.end())
	  vi->second.push_back(value);
	else {
	  stringlist list;
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

	  try {
	    Exiv2::XmpKey key = xmp_key_read(key_string);
	    Exiv2::Value::AutoPtr value = Exiv2::Value::create(Exiv2::xmpText);
	    value->read(value_string);
	    std::cerr << "\tXMP \"" << key << "\" = \"" << *value << "\"" << std::endl;
	    _XMPtags.add(key, value.get());
	  } catch (Exiv2::Error& e) {
	    std::cerr << "** XMP key \"" << key_string << "\" not accepted **" << std::endl;
	  }
	} else 	if (line.substr(start, 4) == "IPTC") {
	  std::string key_string = line.substr(start, eq - start);

	  try {
	    Exiv2::IptcKey key = iptc_key_read(key_string);
	    Exiv2::Value::AutoPtr value = Exiv2::Value::create(Exiv2::asciiString);
	    value->read(value_string);
	    std::cerr << "\tIPTC \"" << key << "\" = \"" << *value << "\"" << std::endl;
	    _IPTCtags.add(key, value.get());
	  } catch (Exiv2::Error& e) {
	    std::cerr << "** IPTC key \"" << key_string << "\" not accepted **" << std::endl;
	  }
	} else {
	  std::string key_string = line.substr(start, eq - start);

	  try {
	    Exiv2::ExifKey key = exif_key_read(key_string);
	    Exiv2::Value::AutoPtr value = exif_value_read(key, value_string);

	    std::cerr << "\tEXIF \"" << key << "\" (" << key.defaultTypeId() << ") = \"" << *value << "\"" << std::endl;
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

  void Tags::extract(fs::path filepath) {
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filepath.native());
    assert(image.get() != 0);

    for (auto ei : image->exifData())
      _EXIFtags.add(ei);

    for (auto ii : image->iptcData())
      _IPTCtags.add(ii);

    for (auto xi : image->xmpData())
      _XMPtags.add(xi);
  }

  void Tags::make_thumbnail(Image::ptr img, const D_thumbnail& dt) {
#ifdef HAZ_JPEG
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
    auto thumbimage = frame.crop_resize(img, D_resize::lanczos(3.0));

    auto dest = std::make_shared<Destination>();
    dest->set_jpeg(D_JPEG(50, 1, 1, false));
    dest->set_depth(8);

    JPEGfile thumbfile("");

    cmsUInt32Number dest_type = thumbfile.preferred_type(dest->modify_type(img->type()));
    cmsHPROFILE dest_profile = dest->get_profile(COLORSPACE_SH(PT_RGB));
    thumbimage->transform_colour_inplace(dest_profile, dest_type);

    std::ostringstream oss;
    thumbfile.write(oss, thumbimage, dest);

    Exiv2::ExifThumb EXIFthumb(_EXIFtags);
    EXIFthumb.setJpegThumbnail((unsigned char*)oss.str().data(), oss.str().length());

    std::cerr << "Done." << std::endl;
#endif
  }

  Exiv2::URational closest_URational(double value);

  void Tags::add_resolution(Image::ptr img) {
    Exiv2::URationalValue v;
    if (img->xres().defined()) {
      v = Exiv2::URationalValue(closest_URational(img->xres()));
      std::cerr << "\tSetting X resolution to " << v.value_[0].first << " ÷ " << v.value_[0].second << " (" << img->xres() << ") ppi." << std::endl;
      try {
	_EXIFtags["Exif.Image.XResolution"] = v;
      } catch (Exiv2::Error& e) {
	std::cerr << "** EXIF key \"Exif.Image.XResolution\" not accepted **" << std::endl;
      }
    }
    if (img->yres().defined()) {
      v = Exiv2::URationalValue(closest_URational(img->yres()));
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
