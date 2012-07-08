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

  typedef std::map<std::string, std::string> hash;

  //! Reads and holds tag information
  class Tags {
  private:
    std::map<std::string, std::string> _variables;
    Exiv2::ExifData _EXIFtags;
    Exiv2::IptcData _IPTCtags;
    Exiv2::XmpData _XMPtags;

  public:
    //! Empty Constructor
    Tags();

    //! Constructor with a filepath from which to load tags (calls Load)
    Tags(fs::path filepath);

    //! Accessor for the internal map of variables.
    inline hash& variables(void) { return _variables; }

    //! Accessor for the internal Exiv2::ExifData object.
    inline Exiv2::ExifData& EXIFtags(void) { return _EXIFtags; }

    //! Accessor for the internal Exiv2::IptcData object.
    inline Exiv2::IptcData& IPTCtags(void) { return _IPTCtags; }

    //! Accessor for the internal Exiv2::XmpData object.
    inline Exiv2::XmpData& XMPtags(void) { return _XMPtags; }

    //! Load tags from supplied file path
    void Load(fs::path filepath);

    //! Embed EXIF/IPTC/XMP tags into an image file
    void Embed(fs::path filepath) const;
  };

}

#endif /* __TAGS_HH__ */
