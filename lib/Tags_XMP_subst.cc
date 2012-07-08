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

namespace PhotoFinish {

  //! Map from Image::Exiftool tag names to Exiv2's tag names
  void populate_XMP_subst(std::map<std::string, std::string>& table) {
    table["XMP:Copyright"] = "";
    table["XMP:Creator"] = "";
    table["XMP:CreatorContactInfoCiAdrCity"] = "";
    table["XMP:CreatorContactInfoCiAdrCtry"] = "";
    table["XMP:CreatorContactInfoCiAdrPcode"] = "";

    table["XMP-cc:License"] = "";

    table["XMP-microsoft:CameraSerialNumber"] = "Xmp.MicrosoftPhoto.CameraSerialNumber";
    table["XMP-microsoft:LensManufacturer"] = "Xmp.MicrosoftPhoto.LensManufacturer";
    table["XMP-microsoft:LensModel"] = "Xmp.MicrosoftPhoto.LensModel";
  }

}
