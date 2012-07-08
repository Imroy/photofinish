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
#include "Tags.hh"

namespace PhotoFinish {

  //! Map from Image::Exiftool tag names to Exiv2's tag names
  void populate_EXIF_subst(hash& table) {

    table["Aperture"]			= "Exif.Image.ApertureValue";
    table["Artist"]			= "Exif.Image.Artist";
    table["BWMode"]			= "";
    table["CameraID"]			= "";
    table["CameraSerialNumber"]		= "Exif.Image.CameraSerialNumber";
    table["CameraType"]			= "";
    table["CircleOfConfusion"]		= "";
    table["ColorSpace"]			= "";
    table["ComponentsConfiguration"]	= "";
    table["CompressedBitsPerPixel"]	= "";
    table["Compression"]		= "";
    table["Contrast"]			= "";
    table["Copyright"]			= "Exif.Image.Copyright";
    table["CreateDate"]			= "";
    table["CustomRendered"]		= "";
    table["DataDump"]			= "";
    table["DigitalZoom"]		= "";
    table["ExifIFD:Lens"]		= "";	// ???
    table["ExifImageLength"]		= "Exif.Image.ImageLength";
    table["ExifImageWidth"]		= "Exif.Image.ImageWidth";
    table["ExifVersion"]		= "Exif.Photo.ExifVersion";
    table["ExposureCompensation"]	= "";
    table["FileSource"]			= "";
    table["FlashpixVersion"]		= "";
    table["FocalPlaneDiagonal"]		= "";
    table["GainControl"]		= "";
    table["HyperfocalDistance"]		= "";
    table["ImageDescription"]		= "";
    table["ImageHeight"]		= "";
    table["ImageSize"]			= "";
    table["ImageWidth"]			= "";
    table["InteropIndex"]		= "";
    table["InteropVersion"]		= "";
    table["LensDistortionParams"]	= "";
    table["LightSource"]		= "";
    table["LightValue"]			= "";
    table["Macro"]			= "";
    table["Make"]			= "Exif.Image.Make";
    table["MaxApertureValue"]		= "Exif.Image.MaxApertureValue";
    table["Model"]			= "Exif.Image.Model";
    table["ModifyDate"]			= "";
    table["OneTouchWB"]			= "";
    table["Orientation"]		= "";
    table["PreCaptureFrames"]		= "";
    table["Quality"]			= "";
    table["Resolution"]			= "";
    table["ResolutionUnit"]		= "Exif.Image.ResolutionUnit";
    table["Saturation"]			= "";
    table["ScaleFactor35efl"]		= "";
    table["SceneCaptureType"]		= "";
    table["Sharpness"]			= "";
    table["ShutterSpeed"]		= "Exif.Image.ShutterSpeedValue";
    table["Software"]			= "Exif.Image.Software";
    table["SpecialMode"]		= "";
    table["TagsFromFile"]		= "";
    table["ThumbnailLength"]		= "";
    table["ThumbnailOffset"]		= "";
    table["UserComment"]		= "Exif.Photo.UserComment";
    table["WhiteBalance"]		= "";
    table["XResolution"]		= "Exif.Image.XResolution";
    table["YCbCrPositioning"]		= "";
    table["YResolution"]		= "Exif.Image.YResolution";

    table["DateTimeOriginal"]		= "Exif.Photo.DateTimeOriginal";
    table["DigitalZoomRatio"]		= "Exif.Photo.DigitalZoomRatio";
    table["ExifIFD:Flash"]		= "Exif.Photo.Flash";
    table["ExifIFD:ISO"]		= "Exif.Photo.ISOSpeedRatings";
    table["ExifIFD:MeteringMode"]	= "Exif.Photo.MeteringMode";
    table["ExposureIndex"]		= "Exif.Photo.ExposureIndex";
    table["ExposureMode"]		= "Exif.Photo.ExposureMode";
    table["ExposureProgram"]		= "Exif.Photo.ExposureProgram";
    table["ExposureTime"]		= "Exif.Photo.ExposureTime";
    table["Flash"]			= "Exif.Photo.Flash";
    table["FNumber"]			= "Exif.Photo.FNumber";
    table["FocalLength"]		= "Exif.Photo.FocalLength";
    table["FocalLength35efl"]		= "Exif.Photo.FocalLengthIn35mmFilm";
    table["FocalLengthIn35mmFormat"]	= "Exif.Photo.FocalLengthIn35mmFilm";
    table["ISO"]			= "Exif.Photo.ISOSpeedRatings";
    table["LensSerialNumber"]		= "Exif.Photo.LensSerialNumber";
    table["MeteringMode"]		= "Exif.Photo.MeteringMode";
    table["SceneType"]			= "Exif.Photo.SceneType";

    table["GPSAltitude"]		= "Exif.GPSInfo.GPSAltitude";
    table["GPSAltitudeRef"]		= "Exif.GPSInfo.GPSAltitudeRef";
    table["GPSDateStamp"]		= "Exif.GPSInfo.GPSDateStamp";
    table["GPSDifferential"]		= "Exif.GPSInfo.GPSDifferential";
    table["GPSDOP"]			= "Exif.GPSInfo.GPSDOP";
    table["GPSLatitude"]		= "Exif.GPSInfo.GPSLatitude";
    table["GPSLatitudeRef"]		= "Exif.GPSInfo.GPSLatitudeRef";
    table["GPSLongitude"]		= "Exif.GPSInfo.GPSLongitude";
    table["GPSLongitudeRef"]		= "Exif.GPSInfo.GPSLongitudeRef";
    table["GPSMeasureMode"]		= "Exif.GPSInfo.GPSMeasureMode";
    table["GPSTimeStamp"]		= "Exif.GPSInfo.GPSTimeStamp";
  }

}


