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

#include "yaml-cpp/yaml.h"
#include <string>
#include <memory>
#include <lcms2.h>
#include <boost/filesystem.hpp>
#include "Image.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  typedef std::map<std::string, std::string> hash;

  //! Sharpen parameters for destination
  class D_sharpen {
  private:
    bool _has_radius, _has_sigma;
    double _radius, _sigma;

  public:
    //! Empty constructor
    D_sharpen();

    inline bool has_radius(void) const { return _has_radius; }
    inline double radius(void) const { return _radius; }
    inline bool has_sigma(void) const { return _has_sigma; }
    inline double sigma(void) const { return _sigma; }

    friend void operator >> (const YAML::Node& node, D_sharpen& ds);
  };

  //! Resize parameters for destination
  class D_resize {
  private:
    bool _has_filter, _has_support;
    std::string _filter;
    double _support;

  public:
    //! Empty constructor
    D_resize();

    //! Constructor
    /*!
      \param f Name of filter
      \param s Support i.e usually the radius of the filter
    */
    D_resize(std::string f, double s);

    inline bool has_filter(void) const { return _has_filter; }
    inline std::string filter(void) const { return _filter; }
    inline bool has_support(void) const { return _has_support; }
    inline double support(void) const { return _support; }

    friend void operator >> (const YAML::Node& node, D_resize& dr);
  };

  //! Target parameters for destination
  class D_target {
  protected:
    std::string _name;
    bool _has_width, _has_height;
    double _width, _height;

  public:
    D_target(std::string n, double w, double h);
    D_target(std::string &n);

    inline std::string name(void) const { return _name; }
    inline bool has_width(void) const { return _has_width; }
    inline double width(void) const { return _width; }
    inline bool has_height(void) const { return _has_height; }
    inline double height(void) const { return _height; }

    friend void operator >> (const YAML::Node& node, D_target& dt);

    typedef std::shared_ptr<D_target> ptr;
  };

  //! JPEG parameters for destination
  class D_JPEG {
  private:
    bool _has_quality, _has_sample, _has_progressive;
    int _quality;
    char _sample_h, _sample_v;
    bool _progressive;

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
    bool add_variables(hash& vars);

    inline bool has_quality(void) const { return _has_quality; }
    inline int quality(void) const { return _quality; }
    inline bool has_sample(void) const { return _has_sample; }
    inline char sample_h(void) const { return _sample_h; }
    inline char sample_v(void) const { return _sample_v; }
    inline bool has_progressive(void) const { return _has_progressive; }
    inline bool progressive(void) const { return _progressive; }

    friend void operator >> (const YAML::Node& node, D_JPEG& dj);
  };

  //! PNG parameters for destination
  class D_PNG {
  private:

  public:
    D_PNG();

    friend void operator >> (const YAML::Node& node, D_PNG& dp);
  };

  //! ICC profile parameters for destination
  class D_profile {
  private:
    bool _has_name, _has_filepath;
    std::string _name;
    fs::path _filepath;

  public:
    D_profile();

    inline bool has_name(void) const { return _has_name; }
    inline std::string name(void) const { return _name; }
    inline bool has_filepath(void) const { return _has_filepath; }
    inline fs::path filepath(void) const { return _filepath; }

    friend void operator >> (const YAML::Node& node, D_profile& dp);
  };

  //! Thumbnail parameters for destination
  class D_thumbnail {
  private:
    bool _has_generate, _generate;
    bool _has_maxwidth, _has_maxheight;
    double _maxwidth, _maxheight;

  public:
    D_thumbnail();

    inline bool has_generate(void) const { return _has_generate; }
    inline bool generate(void) const { return _generate; }
    inline bool has_maxwidth(void) const { return _has_maxwidth; }
    inline double maxwidth(void) const { return _maxwidth; }
    inline bool has_maxheight(void) const { return _has_maxheight; }
    inline double maxheight(void) const { return _maxheight; }

    friend void operator >> (const YAML::Node& node, D_thumbnail& dt);
  };

}

#endif // __DESTINATION_ITEMS_HH__
