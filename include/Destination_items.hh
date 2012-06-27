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
#include <lcms2.h>
#include "Image.hh"

using namespace std;

class D_sharpen {
private:
  double _radius, _sigma;

public:
  D_sharpen();
  D_sharpen(const D_sharpen& other);
  ~D_sharpen();

  inline double radius(void) const { return _radius; }
  inline double sigma(void) const { return _sigma; }

  friend void operator >> (const YAML::Node& node, D_sharpen& ds);
};

class D_resize {
private:
  bool _has_filter;
  string _filter;
  bool _has_support;
  double _support;

public:
  D_resize();
  D_resize(const D_resize& other);
  ~D_resize();

  inline bool has_filter(void) const { return _has_filter; }
  inline string filter(void) const { return _filter; }
  inline bool has_support(void) const { return _has_support; }
  inline double support(void) const { return _support; }

  friend void operator >> (const YAML::Node& node, D_resize& dr);
};

class D_target {
protected:
  string _name;
  double _width, _height;

public:
  D_target(string &n);
  D_target(const D_target& other);
  ~D_target();

  inline string name(void) const { return _name; }
  inline double width(void) const { return _width; }
  inline double height(void) const { return _height; }

  friend void operator >> (const YAML::Node& node, D_target& dt);
};

class D_JPEG {
private:
  int _quality;
  char _sample_h, _sample_v;
  bool _progressive;

public:
  D_JPEG();
  D_JPEG(const D_JPEG& other);
  ~D_JPEG();

  inline int quality(void) const { return _quality; }
  inline char sample_h(void) const { return _sample_h; }
  inline char sample_v(void) const { return _sample_v; }
  inline bool progressive(void) const { return _progressive; }

  friend void operator >> (const YAML::Node& node, D_JPEG& dj);
};

class D_PNG {
private:

public:
  D_PNG();
  D_PNG(const D_PNG& other);
  ~D_PNG();

  friend void operator >> (const YAML::Node& node, D_PNG& dp);
};

class D_profile {
private:
  string _name, _filepath;
  cmsHPROFILE _profile;

public:
  D_profile();
  D_profile(const D_profile& other);
  ~D_profile();

  inline string name(void) const { return _name; }
  inline string filepath(void) const { return _filepath; }
  inline bool has_profle(void) const { return _profile != NULL; }
  inline cmsHPROFILE profile(void) const { return _profile; }

  friend void operator >> (const YAML::Node& node, D_profile& dp);
};

#endif // __DESTINATION_ITEMS_HH__
