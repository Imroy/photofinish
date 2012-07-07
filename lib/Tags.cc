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
#include "Tags.hh"
#include <exiv2/exiv2.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <map>

namespace PhotoFinish {

  Tags::Tags(fs::path filepath) {
    Load(filepath);
  }

  void Tags::Load(fs::path filepath) {
    std::cerr << "Tags::Load(): Loading \"" << filepath.native() << "\"..." << std::endl;
    std::ifstream fin(filepath.native());
    if (fin.fail()) {
      std::cerr << "Tags::Load(): Could not open \"" << filepath.native() << "\"" << std::endl;
      return;
    }

    while (!fin.eof()) {
      std::string line;
      getline(fin, line);
      if (fin.eof())
	break;

      if (line.substr(0, 2) == "#@") {
	int start = line.find_first_not_of(" \t", 2);
	int end = line.find_last_not_of(" \t");
	std::cerr << "Tags::Load(): line=\"" << line << "\", start=" << start << ", end=" << end << std::endl;
	Load(".tags" / fs::path(line.substr(start, end + 1 - start)));
	continue;
      }

      if (line.substr(0, 2) == "#$") {
	int start = line.find_first_not_of(" \t", 2);
	int eq = line.find_first_of('=', start);
	int end = line.find_last_not_of(" \t");
	std::cerr << "Tags::Load(): line=\"" << line << "\", start=" << start << ", eq=" << eq << ", end=" << end << std::endl;
	std::string key = line.substr(start, eq - start);
	std::string v = line.substr(eq + 1, end - eq);
	std::cerr << "Tags::Load(): Adding variable \"" << key << "\", value \"" << v << "\"" << std::endl;
	_variables.insert(std::pair<std::string, std::string>(key, v));
	continue;
      }

      if (line.substr(0, 1) == "-") {
	int start = line.find_first_not_of(" \t", 1);
	int eq = line.find_first_of('=', start);
	int end = line.find_last_not_of(" \t");
	std::cerr << "Tags::Load(): line=\"" << line << "\", start=" << start << ", eq=" << eq << ", end=" << end << std::endl;
	if (line.substr(start, 4) == "XMP-") {
	  std::string key_string = line.substr(start, eq - start).replace(0, 4, "Xmp.");
	  int x;
	  while ((x = key_string.find_first_of(':')) > 0) {
	    std::cerr << "Tags::Load(): key_string=\"" << key_string << "\", x=" << x << std::endl;
	    key_string = key_string.replace(x, 1, ".");
	  }
	  /*
	  Exiv2::XmpKey key(key_string);
	  Exiv2::Value::AutoPtr v = Exiv2::Value::create(Exiv2::xmpText);
	  v->read(line.substr(eq + 1, end - eq));
	  std::cerr << "Tags::Load(): Adding XMP \"" << key << "\", value \"" << *v << "\"" << std::endl;
	  _XMPtags.add(key, v.get());
	  */
	} else {
	  /*
	  Exiv2::ExifKey key("Exif." + line.substr(start, eq - start));
	  Exiv2::Value::AutoPtr v = Exiv2::Value::create(Exiv2::asciiString);
	  v->read(line.substr(eq + 1, end - eq));
	  std::cerr << "Tags::Load(): Adding EXIF \"" << key << "\", value \"" << *v << "\"" << std::endl;
	  _EXIFtags.add(key, v.get());
	  */
	}
	continue;
      }

      std::cerr << "Tags::Load(): Unrecognised line \"" << line << "\" in file \"" << filepath.native() << "\"" << std::endl;
    }
    fin.close();
    std::cerr << "Tags::Load(): Finished reading \"" << filepath.native() << "\"" << std::endl;
  }


}
