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
#include <memory>
#include <list>
#include "Image.hh"
#include "ImageFile.hh"
#include "Destination.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  //! Reads and holds tag information
  class Tags {
  private:
    std::list<fs::path> _searchpaths;
    multihash _variables;
    Exiv2::ExifData _EXIFtags;
    Exiv2::IptcData _IPTCtags;
    Exiv2::XmpData _XMPtags;

  public:
    //! Empty Constructor
    Tags();

    //! Copy constructor
    Tags(const Tags &other);

    //! Constructor with a filepath from which to load tags (calls Load)
    Tags(fs::path filepath);

    //! Shared pointer for a Tags object
    typedef std::shared_ptr<Tags> ptr;

    //! Duplicate the tags
    ptr dupe(void) const;

    inline void add_searchpath(fs::path path) { _searchpaths.push_back(path); }

    //! The map of variables.
    inline multihash& variables(void) { return _variables; }

    //! The Exiv2::ExifData object.
    inline Exiv2::ExifData& EXIFtags(void) { return _EXIFtags; }

    //! The Exiv2::IptcData object.
    inline Exiv2::IptcData& IPTCtags(void) { return _IPTCtags; }

    //! The Exiv2::XmpData object.
    inline Exiv2::XmpData& XMPtags(void) { return _XMPtags; }

    //! Try to load tags from a file, looking in the search paths
    //! \return if the file was found and loaded
    bool try_load(fs::path filepath);

    //! Load tags from supplied file path
    void load(fs::path filepath);

    //! Extract EXIF/IPTC/XMP tags from an image file
    void extract(fs::path filepath);

    //! Extract EXIF/IPTC/XMP tags from an image file
    void extract(ImageFilepath& imagefilepath) { this->extract(imagefilepath.filepath()); }

    //! Create a thumbnail from the supplied image
    void add_thumbnail_chain(ImageSource::ptr source, WorkGang::ptr workgang, const D_thumbnail& dt);

    void add_resolution(ImageHeader::ptr header);

    //! Embed EXIF/IPTC/XMP tags into an image file
    void embed(fs::path filepath) const;

    //! Embed EXIF/IPTC/XMP tags into an image file
    inline void embed(ImageFilepath& imagefilepath) const { this->embed(imagefilepath.filepath()); }
  };

}

#endif /* __TAGS_HH__ */
