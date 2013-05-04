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
#include "Tags.hh"

namespace PhotoFinish {

  //! Map from Image::Exiftool tag names to Exiv2's tag names
  subst_table XMP_key_subst = {
    StrPair("XMP:Copyright",				"Xmp.dc.Copyright"), // ?
    StrPair("XMP:Creator",				"Xmp.dc.Creator"),

    StrPair("XMP:CreatorContactInfoCiAdrCity",		"Xmp.iptc.CiAdrCity"),
    StrPair("XMP:CreatorContactInfoCiAdrCtry",		"Xmp.iptc.CiAdrCtry"),
    StrPair("XMP:CreatorContactInfoCiAdrExtadr",	"Xmp.iptc.CiAdrExtadr"),
    StrPair("XMP:CreatorContactInfoCiAdrPcode",		"Xmp.iptc.CiAdrPcode"),

    StrPair("XMP-cc:License",				"Xmp.cc.License"), // Creative Commons not supported in Exiv2 yet?

    StrPair("XMP-microsoft:CameraSerialNumber",		"Xmp.MicrosoftPhoto.CameraSerialNumber"),
    StrPair("XMP-microsoft:LensManufacturer",		"Xmp.MicrosoftPhoto.LensManufacturer"),
    StrPair("XMP-microsoft:LensModel",			"Xmp.MicrosoftPhoto.LensModel"),
  };

  Exiv2::XmpKey xmp_key_read(std::string key_string) {
    for (auto i : XMP_key_subst)
      if (boost::iequals(key_string, i.first)) {
	key_string = i.second;
	break;
      }

    return Exiv2::XmpKey(key_string);
  }

}
