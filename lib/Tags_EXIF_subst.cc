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
  void populate_EXIF_subst(std::map<std::string, std::string>& table) {
    table["Aperture"] = "";
    table["Artist"] = "Exif.Image.Artist";
    table["BWMode"] = "";
    table["CameraID"] = "";
    table["CameraSerialNumber"] = "Exif.Image.CameraSerialNumber";
    table["CameraType"] = "";
    table["CircleOfConfusion"] = "";
    table["ColorSpace"] = "";
    table["ComponentsConfiguration"] = "";
    table["CompressedBitsPerPixel"] = "";
    table["Compression"] = "";
    table["Contrast"] = "";
    table["Copyright"] = "Exif.Image.Copyright";
    table["CreateDate"] = "";
    table["CustomRendered"] = "";
    table["DataDump"] = "";
    table["DigitalZoom"] = "";
    table["DigitalZoomRatio"] = "";
    table["ExifIFD:Flash"] = "";
    table["ExifIFD:ISO"] = "";
    table["ExifIFD:Lens"] = "";
    table["ExifImageLength"] = "";
    table["ExifImageWidth"] = "";
    table["ExifVersion"] = "";
    table["ExposureCompensation"] = "";
    table["ExposureMode"] = "";
    table["ExposureTime"] = "";
    table["FileSource"] = "";
    table["Flash"] = "";
    table["FlashpixVersion"] = "";
    table["FNumber"] = "";
    table["FocalLength35efl"] = "";
    table["FocalPlaneDiagonal"] = "";
    table["GainControl"] = "";
    table["HyperfocalDistance"] = "";
    table["ImageDescription"] = "";
    table["ImageHeight"] = "";
    table["ImageSize"] = "";
    table["ImageWidth"] = "";
    table["InteropIndex"] = "";
    table["InteropVersion"] = "";
    table["ISOSetting"] = "";
    table["LensDistortionParams"] = "";
    table["LensSerialNumber"] = "";
    table["LightSource"] = "";
    table["LightValue"] = "";
    table["Macro"] = "";
    table["Make"] = "Image.Make";
    table["MaxApertureValue"] = "";
    table["MeteringMode"] = "";
    table["Model"] = "Image.Model";
    table["ModifyDate"] = "";
    table["OneTouchWB"] = "";
    table["Orientation"] = "";
    table["PreCaptureFrames"] = "";
    table["Quality"] = "";
    table["Resolution"] = "";
    table["ResolutionUnit"] = "Exif.Image.ResolutionUnit";
    table["Saturation"] = "";
    table["ScaleFactor35efl"] = "";
    table["SceneCaptureType"] = "";
    table["Sharpness"] = "";
    table["ShutterSpeed"] = "";
    table["Software"] = "";
    table["SpecialMode"] = "";
    table["TagsFromFile"] = "";
    table["ThumbnailLength"] = "";
    table["ThumbnailOffset"] = "";
    table["UserComment"] = "";
    table["WhiteBalance"] = "";
    table["XResolution"] = "Exif.Image.XResolution";
    table["YCbCrPositioning"] = "";
    table["YResolution"] = "Exif.Image.YResolution";

    table["DateTimeOriginal"] = "Exif.Photo.DateTimeOriginal";
    table["ExposureProgram"] = "Exif.Photo.ExposureProgram";
    table["FocalLength"] = "Exif.Photo.FocalLength";
    table["FocalLengthIn35mmFormat"] = "Exif.Photo.FocalLengthIn35mmFilm";
    table["ExifIFD:MeteringMode"] = "Exif.Photo.MeteringMode";
    table["ExposureIndex"] = "Exif.Photo.ExposureIndex";
    table["ISO"] = "Exif.Photo.ISOSpeedRatings";
    table["SceneType"] = "Exif.Photo.SceneType";

    table["GPSAltitude"] = "Exif.GPSInfo.GPSAltitude";
    table["GPSAltitudeRef"] = "Exif.GPSInfo.GPSAltitudeRef";
    table["GPSDateStamp"] = "Exif.GPSInfo.GPSDateStamp";
    table["GPSDifferential"] = "Exif.GPSInfo.GPSDifferential";
    table["GPSDOP"] = "Exif.GPSInfo.GPSDOP";
    table["GPSLatitude"] = "Exif.GPSInfo.GPSLatitude";
    table["GPSLatitudeRef"] = "Exif.GPSInfo.GPSLatitudeRef";
    table["GPSLongitude"] = "Exif.GPSInfo.GPSLongitude";
    table["GPSLongitudeRef"] = "Exif.GPSInfo.GPSLongitudeRef";
    table["GPSMeasureMode"] = "Exif.GPSInfo.GPSMeasureMode";
    table["GPSTimeStamp"] = "Exif.GPSInfo.GPSTimeStamp";
  }

}


