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
#ifndef __DESTINATION_HH__
#define __DESTINATION_HH__

#include "yaml-cpp/yaml.h"
#include <string>
#include <map>
#include <boost/filesystem.hpp>
#include "Destination_items.hh"
#include "Image.hh"
#include "Frame.hh"
#include "Definable.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  //! Read a boolean value from a YAML file. Why doesn't yaml-cpp define this ?
  void operator >> (const YAML::Node& node, bool& b);

  //! Read a boost.filesystem path from a YAML file
  void operator >> (const YAML::Node& node, fs::path& p);

  //! Represents a destination, read from destinations.yml
  class Destination {
  private:
    definable<std::string> _name;	//! Name of this destination
    definable<fs::path> _dir;		//! Destination directory

    definable<double> _size;			//! Size of long edge in inches (yuck)

    D_sharpen _sharpen;
    D_resize _resize;

    std::map<std::string, D_target::ptr> _targets;	//! List of targets

    definable<std::string> _format;	//! Output format

    definable<int> _depth;			//! Output bit depth

    definable<bool> _noresize;

    D_JPEG _jpeg;
    D_PNG _png;
    D_TIFF _tiff;

    definable<cmsUInt32Number> _intent;	//! CMS rendering intent

    D_profile::ptr _profile;

    definable<bool> _forcergb;	//! Force the output to be RGB
    definable<bool> _forcegrey;	//! Force the output to be greyscale

    D_thumbnail _thumbnail;

    multihash _variables;

  public:
    //! Empty constructor
    Destination();

    //! Copy constructor
    Destination(const Destination& other);

    //! Destructor
    ~Destination();

    //! Assignment operator
    Destination& operator=(const Destination& b);

    typedef std::shared_ptr<Destination> ptr;

    //! Duplicate
    inline ptr dupe(void) { return Destination::ptr(new Destination(*this)); }

    //! Duplicate the current object and incorporate variables
    ptr add_variables(multihash& vars);

    //! Find the best crop+rescaling frame for an image
    Frame::ptr best_frame(Image::ptr img);

    inline definable<std::string> name(void) const { return _name; }

    inline const definable<fs::path>& dir(void) const { return _dir; }

    inline definable<double> size(void) const { return _size; }

    inline const D_sharpen& sharpen(void) const { return _sharpen; }

    inline const D_resize& resize(void) const { return _resize; }

    inline int num_targets(void) const { return _targets.size(); }
    inline bool has_targets(void) const { return !_targets.empty(); }
    inline const std::map<std::string, D_target::ptr>& targets(void) const { return _targets; }

    inline definable<std::string> format(void) const { return _format; }

    inline definable<int> depth(void) const { return _depth; }
    inline void set_depth(int d) { _depth = d; }

    inline definable<bool> noresize(void) const { return _noresize; }

    inline const D_JPEG& jpeg(void) const { return _jpeg; }
    inline void set_jpeg(const D_JPEG& j) { _jpeg = j; }

    inline const D_PNG& png(void) const { return _png; }

    inline const D_TIFF& tiff(void) const { return _tiff; }

    inline definable<cmsUInt32Number> intent(void) const { return _intent; }

    inline const D_profile::ptr profile(void) const { return _profile; }
    inline void set_profile(std::string name, fs::path filepath) { _profile = D_profile::ptr(new D_profile(name, filepath)); }
    inline void set_profile(std::string name, void *data, unsigned int data_size) { _profile = D_profile::ptr(new D_profile(name, data, data_size)); }

    inline definable<bool> forcergb(void) const { return _forcergb; }
    inline definable<bool> forcegrey(void) const { return _forcegrey; }

    inline const D_thumbnail& thumbnail(void) const { return _thumbnail; }

    //! Read a destination record from a YAML document
    friend void operator >> (const YAML::Node& node, Destination& d);
  };

  //! A wrapper class for reading destinations from a YAML file and storing them in a map
  class Destinations {
  private:
    std::map<std::string, Destination::ptr> _destinations;

  public:
    Destinations(fs::path filepath);
    Destinations(const Destinations& other);
    ~Destinations();

    Destinations& operator=(const Destinations& b);

    void Load(fs::path filepath);

    typedef std::map<std::string, Destination::ptr>::iterator iterator;
    typedef std::map<std::string, Destination::ptr>::const_iterator const_iterator;

    inline std::map<std::string, Destination::ptr>::size_type count(const std::string& key) const { return _destinations.count(key); }

    inline iterator begin(void) { return _destinations.begin(); }
    inline const_iterator begin(void) const { return _destinations.begin(); }

    inline iterator end(void) { return _destinations.end(); }
    inline const_iterator end(void) const { return _destinations.end(); }

    inline Destination::ptr operator[] (const std::string& key) { return _destinations[key]; }
  };

}

#endif // __DESTINATION_HH__
