/*
	Copyright 2014-2019 Ian Tester

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
#pragma once

#include <exiv2/exiv2.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <list>
#include "Image.hh"
#include "Destination.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  typedef std::vector<std::pair<std::string, std::string> > subst_table;
#define StrPair(s, v) std::make_pair<std::string, std::string>(s, v)

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
    Tags(const fs::path& filepath);

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

    //! Copy EXIF/IPTC/XMP tags from an image
    void copy_from(Image::ptr img);

    //! Create a thumbnail from the supplied image
    void make_thumbnail(Image::ptr img, const D_thumbnail& dt);

    void add_resolution(Image::ptr img);

    //! Copy EXIF/IPTC/XMP tags to an image
    void copy_to(Image::ptr img) const;

  };

  //! Find a close rational fraction given a floating-point value
  template <typename Num_type, typename R_type>
  Exiv2::ValueType<R_type>& closest_Rational(double value) {
    double margin = fabs(value) * 1e-6;
    Num_type num = 0;
    Num_type den;
    for (den = 1; den < INT_MAX; den++) {
      double numf = value * den;
      if ((numf < INT_MIN) || (numf > INT_MAX))
	break;

      num = round(numf);
      double error = fabs(num - numf);
      if (error < margin * den)
	break;
    }

    Exiv2::ValueType<R_type> *rv = new Exiv2::ValueType<R_type>(R_type(num, den));
    return *rv;
  }

}
