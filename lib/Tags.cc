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
#include <math.h>
#include <exiv2/exiv2.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
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
	std::string v = line.substr(eq + 1, end - eq);
	std::cerr << "\tVariable \"" << key << "\" = \"" << v << "\"" << std::endl;
	_variables.insert(std::pair<std::string, std::string>(key, v));
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

	if (line.substr(start, 3) == "XMP") {
	  std::string key_string = line.substr(start, eq - start);
	  hash::iterator si;
	  if (((si = XMP_subst.find(key_string)) != XMP_subst.end()) && (si->second.length() > 0))
	    key_string = si->second;

	  try {
	    Exiv2::XmpKey key(key_string);
	    Exiv2::Value::AutoPtr v = Exiv2::Value::create(Exiv2::xmpText);
	    v->read(line.substr(eq + 1, end - eq));
	    std::cerr << "\tXMP \"" << key << "\" = \"" << *v << "\"" << std::endl;
	    _XMPtags.add(key, v.get());
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
	    Exiv2::Value::AutoPtr v = Exiv2::Value::create(Exiv2::asciiString);
	    v->read(line.substr(eq + 1, end - eq));
	    std::cerr << "\tIPTC \"" << key << "\" = \"" << *v << "\"" << std::endl;
	    _IPTCtags.add(key, v.get());
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
	    Exiv2::Value::AutoPtr v = Exiv2::Value::create(Exiv2::asciiString);
	    v->read(line.substr(eq + 1, end - eq));
	    std::cerr << "\tEXIF \"" << key << "\" = \"" << *v << "\"" << std::endl;
	    _EXIFtags.add(key, v.get());
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

    Frame frame(width, height, 0, 0, img->width(), img->height(), 0);

    D_resize dr("Lanczos", 3.0);
    Filter lanczos(dr);
    Image::ptr thumbimage = frame.crop_resize(img, lanczos);

    Destination dest;
    dest.set_jpeg(D_JPEG(50, 1, 1, false));

    JPEGFile thumbfile("");
    std::ostringstream oss;
    thumbfile.write(oss, thumbimage, dest);

    Exiv2::ExifThumb EXIFthumb(_EXIFtags);
    EXIFthumb.setJpegThumbnail((unsigned char*)oss.str().data(), oss.str().length());

    std::cerr << "Done." << std::endl;
  }

  void Tags::add_resolution(Image::ptr img, Destination::ptr dest) {
    double res = (img->width() > img->height() ? img->width() : img->height()) / dest->size();
    unsigned int num, den;
    for (den = 1; den < 65535; den++) {
      num = round(res * den);
      double error = fabs(((double)num / den) - res);
      if (error < res * 0.001) {
	std::cerr << "\tSetting resolution to " << num << " / " << den << " (" << res << " +- " << error << ") ppi." << std::endl;
	break;
      }
    }
    Exiv2::URationalValue v(Exiv2::URational(num, den));
    try {
      Exiv2::ExifKey key("Exif.Image.XResolution");
      _EXIFtags.add(key, &v);
    } catch (Exiv2::Error& e) {
      std::cerr << "** EXIF key \"Exif.Image.XResolution\" not accepted **" << std::endl;
    }
    try {
      Exiv2::ExifKey key("Exif.Image.YResolution");
      _EXIFtags.add(key, &v);
    } catch (Exiv2::Error& e) {
      std::cerr << "** EXIF key \"Exif.Image.YResolution\" not accepted **" << std::endl;
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
