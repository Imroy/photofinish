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
#include <lcms2.h>
#include "Image.hh"
#include "Filter.hh"

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
  string _filter;
  double _support;

public:
  D_resize();
  D_resize(const D_resize& other);
  ~D_resize();

  inline string filter(void) const { return _filter; }
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

class Frame : public D_target {
private:
  // width and height attributes inherited from D_target are the final size of the image
  double _crop_x, _crop_y, _crop_w, _crop_h;	// coordinates for cropping from the original image
  double _resolution;				// PPI

public:
  Frame(const D_target& target, double x, double y, double w, double h, double r);
  ~Frame();

  // Resize the image using a Lanczos filter
  Image* crop_resize(Image* img, _Filter* filter);

  inline double crop_x(void) const { return _crop_x; }
  inline double crop_y(void) const { return _crop_y; }
  inline double crop_w(void) const { return _crop_w; }
  inline double crop_h(void) const { return _crop_h; }
  inline double resolution(void) const { return _resolution; }
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

class Destination {
private:
  string _name, _dir;		// Name of this destination and the destination directory
  double _size;			// Size in inches (yuck)
  bool _has_sharpen, _has_resize;
  D_sharpen _sharpen;
  D_resize _resize;
  map<string, D_target*> _targets;
  string _format;
  int _depth;
  bool _noresize;
  bool _has_jpeg, _has_png;
  D_JPEG _jpeg;
  D_PNG _png;
  cmsUInt32Number _intent;
  bool _has_profile;
  D_profile _profile;

public:
  Destination();
  Destination(const Destination& other);
  ~Destination();

  Frame* best_frame(const Image* img);

  inline string name(void) const { return _name; }
  inline string dir(void) const { return _dir; }
  inline double size(void) const { return _size; }
  inline bool has_sharpen(void) const { return _has_sharpen; }
  inline const D_sharpen& sharpen(void) const { return _sharpen; }
  inline bool has_resize(void) const { return _has_resize; }
  inline const D_resize& resize(void) const { return _resize; }
  inline int num_targets(void) const { return _targets.size(); }
  inline bool has_targets(void) const { return !_targets.empty(); }
  inline const map<string, D_target*>& targets(void) const { return _targets; }
  inline string format(void) const { return _format; }
  inline int depth(void) const { return _depth; }
  inline bool noresize(void) const { return _noresize; }
  inline bool has_png(void) const { return _has_png; }
  inline const D_PNG& png(void) const { return _png; }
  inline bool has_jpeg(void) const { return _has_jpeg; }
  inline const D_JPEG& jpeg(void) const { return _jpeg; }
  inline cmsUInt32Number intent(void) const { return _intent; }
  inline bool has_profile(void) const { return _has_profile; }
  inline const D_profile& profile(void) const { return _profile; }

  friend void operator >> (const YAML::Node& node, Destination& d);
};

#endif // __DESTINATION_HH__
