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
#include "Definable.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  typedef std::map<std::string, std::string> hash;

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
    definable< std::pair<char, char> > _sample;
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
    void add_variables(hash& vars);

    inline definable<int> quality(void) const { return _quality; }
    inline definable< std::pair<char, char> > sample(void) const { return _sample; }
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

  //! ICC profile parameters for destination
  class D_profile : public Role_Definable {
  private:
    definable<std::string> _name;
    definable<fs::path> _filepath;

  public:
    D_profile();

    inline definable<std::string> name(void) const { return _name; }
    inline definable<fs::path> filepath(void) const { return _filepath; }

    friend void operator >> (const YAML::Node& node, D_profile& dp);
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
