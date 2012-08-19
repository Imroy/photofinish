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
#ifndef __DESTINATION_ITEMS_HH__
#define __DESTINATION_ITEMS_HH__

#include <string>
#include <memory>
#include <vector>
#include "yaml-cpp/yaml.h"
#include <lcms2.h>
#include <boost/filesystem.hpp>
#include "Image.hh"
#include "Definable.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  //! Read a boolean value from a YAML file. Why doesn't yaml-cpp define this ?
  void operator >> (const YAML::Node& node, bool& b);

  //! Read a boost.filesystem path from a YAML file
  void operator >> (const YAML::Node& node, fs::path& p);


  //! A simple hash
  typedef std::map<std::string, std::string> hash;

  //! A list of strings
  typedef std::vector<std::string> stringlist;

  //! A hash of string lists
  typedef std::map<std::string, stringlist > multihash;

  //! Sharpen parameters for destination
  class D_sharpen : public Role_Definable {
  private:
    definable<double> _radius, _sigma;

  public:
    //! Empty constructor
    D_sharpen();

    inline definable<double> radius(void) const { return _radius; }
    inline definable<double> sigma(void) const { return _sigma; }

    friend void operator >> (const YAML::Node& node, D_sharpen& ds);
  };

  //! Resize parameters for destination
  class D_resize : public Role_Definable {
  private:
    definable<std::string> _filter;
    definable<double> _support;

    D_resize(std::string f, double s);

  public:
    //! Empty constructor
    D_resize();

    //! Named constructor
    /*! Constructs a D_resize object with filter="lanczos" and the supplied radius
      \param r Radius of Lanczos filter
    */
    static inline D_resize lanczos(double r) { return D_resize("lanczos", r); }

    inline definable<std::string> filter(void) const { return _filter; }
    inline definable<double> support(void) const { return _support; }

    friend void operator >> (const YAML::Node& node, D_resize& dr);
  };

  //! Target parameters for destination
  class D_target {
  protected:
    std::string _name;
    definable<double> _width, _height;
    definable<double> _size;		//! A target-specific size (in inches) to override the one in the destination

  public:
    D_target(std::string n, double w, double h);
    D_target(std::string &n);

    inline std::string name(void) const { return _name; }
    inline definable<double> width(void) const { return _width; }
    inline definable<double> height(void) const { return _height; }
    inline definable<double> size(void) const { return _size; }

    friend void operator >> (const YAML::Node& node, D_target& dt);

    typedef std::shared_ptr<D_target> ptr;
  };

  //! JPEG parameters for destination
  class D_JPEG : public Role_Definable {
  private:
    definable<int> _quality;
    definable< std::pair<int, int> > _sample;
    definable<bool> _progressive;

  public:
    //! Empty constructor
    D_JPEG();

    //! Constructor
    /*!
      \param q Quality
      \param h,v Chroma sampling
      \param p Progressive
    */
    D_JPEG(int q, char h, char v, bool p);

    //! Set values from a map of "variables"
    void add_variables(multihash& vars);

    inline definable<int> quality(void) const { return _quality; }
    inline definable< std::pair<int, int> > sample(void) const { return _sample; }
    inline definable<bool> progressive(void) const { return _progressive; }

    friend void operator >> (const YAML::Node& node, D_JPEG& dj);
  };

  //! PNG parameters for destination
  class D_PNG : public Role_Definable {
  private:

  public:
    D_PNG();

    friend void operator >> (const YAML::Node& node, D_PNG& dp);
  };

  //! TIFF parameters for destination
  class D_TIFF : public Role_Definable {
  private:
    definable<std::string> _artist, _copyright;
    definable<std::string> _compression;

  public:
    //! Empty constructor
    D_TIFF();

    //! Set values from a map of "variables"
    void add_variables(multihash& vars);

    inline definable<std::string> artist(void) const { return _artist; }
    inline definable<std::string> copyright(void) const { return _copyright; }
    inline definable<std::string> compression(void) const { return _compression; }

    friend void operator >> (const YAML::Node& node, D_TIFF& dt);
  };

  //! JP2 parameters for destination
  class D_JP2 : public Role_Definable {
  private:
    definable<int> _numresolutions;
    definable<std::string> _prog_order;
    std::vector<float> _rates;
    definable< std::pair<int, int> > _tile_size;

  public:
    //! Empty constructor
    D_JP2();

    //! Set values from a map of "variables"
    void add_variables(multihash& vars);

    inline definable<int> numresolutions(void) const { return _numresolutions; }
    inline definable<std::string> prog_order(void) const { return _prog_order; }
    inline int num_rates(void) const { return _rates.size(); }
    inline float rate(int n) const { return _rates[n]; }
    inline definable< std::pair<int, int> > tile_size(void) const { return _tile_size; }

    friend void operator >> (const YAML::Node& node, D_JP2& dj);
  };

  //! ICC profile parameters for destination
  class D_profile {
  private:
    definable<std::string> _name;
    definable<fs::path> _filepath;
    void *_data;
    unsigned int _data_size;

  public:
    //! Empty constructor
    D_profile();

    //! Constructor
    D_profile(std::string name, fs::path filepath);

    //! Constructor
    D_profile(std::string name, void *data, unsigned int data_size);

    //! Copy constructor
    D_profile(const D_profile& other);

    //! Destructor
    ~D_profile();

    //! Assignment operator
    D_profile& operator=(const D_profile& b);

    //! Name of the profile
    inline definable<std::string> name(void) const { return _name; }

    //! File path for reading the profile
    inline definable<fs::path> filepath(void) const { return _filepath; }

    //! Do we have the profile data instead of a file path?
    inline bool has_data(void) const { return _data != NULL; }

    //! The profile data for LCMS2
    cmsHPROFILE profile(void) const;

    //! The profile data
    inline void* data(void) const { return _data; }

    //! The size of the profile data
    inline unsigned int data_size(void) const { return _data_size; }

    friend void operator >> (const YAML::Node& node, D_profile& dp);

    //! Shared pointer for a D_profile
    typedef std::shared_ptr<D_profile> ptr;
  };

  //! Thumbnail parameters for destination
  class D_thumbnail : public Role_Definable {
  private:
    definable<bool> _generate;
    definable<double> _maxwidth, _maxheight;

  public:
    D_thumbnail();

    inline definable<bool> generate(void) const { return _generate; }
    inline definable<double> maxwidth(void) const { return _maxwidth; }
    inline definable<double> maxheight(void) const { return _maxheight; }

    friend void operator >> (const YAML::Node& node, D_thumbnail& dt);
  };

}

#endif // __DESTINATION_ITEMS_HH__
