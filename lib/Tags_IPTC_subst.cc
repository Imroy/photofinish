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
  subst_table IPTC_key_subst = {
    StrPair("IPTC:By-line",			"Iptc.Application2.Byline"),
    StrPair("IPTC:City",			"Iptc.Application2.City"),
    StrPair("IPTC:Country-PrimaryLocationCode",	"Iptc.Application2.CountryCode"),
    StrPair("IPTC:Country-PrimaryLocationName",	"Iptc.Application2.CountryName"),
    StrPair("IPTC:CopyrightNotice",		"Iptc.Application2.Copyright"),
    StrPair("IPTC:Province-State",		"Iptc.Application2.ProvinceState"),
    StrPair("IPTC:Sub-location",		"Iptc.Application2.SubLocation"),
  };

  Exiv2::IptcKey iptc_key_read(std::string key_string) {
    for (auto i : IPTC_key_subst)
      if (boost::iequals(key_string, i.first)) {
	key_string = i.second;
	break;
      }

    return Exiv2::IptcKey(key_string);
  }

}
