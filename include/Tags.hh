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
#ifndef __TAGS_HH__
#define __TAGS_HH__

#include <exiv2/exiv2.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <string>
#include <map>

namespace fs = boost::filesystem;

namespace PhotoFinish {

  class Tags {
  private:
    std::map<std::string, std::string> _variables;
    Exiv2::ExifData _EXIFtags;
    Exiv2::XmpData _XMPtags;

  public:
    Tags(fs::path filepath);

    void Load(fs::path filepath);
  };

}

#endif /* __TAGS_HH__ */
