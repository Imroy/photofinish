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
#include <iostream>
#include <iomanip>
#include <fstream>
#include <strings.h>
#include <string.h>
#include <math.h>
#include "Destination_items.hh"
#include "Destination.hh"
#include "Exception.hh"

namespace PhotoFinish {

  void operator >> (const YAML::Node& node, bool& b) {
    int temp;
    node >> temp;
    b = temp > 0 ? true : false;
  }

  void operator >> (const YAML::Node& node, fs::path& p) {
    std::string temp;
    node >> temp;
    p = temp;
  }



  D_sharpen::D_sharpen()
  {}

  //! Read a D_sharpen record from a YAML file
  void operator >> (const YAML::Node& node, D_sharpen& ds) {
    try {
      node["radius"] >> ds._radius;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["sigma"] >> ds._sigma;
    } catch(YAML::RepresentationException& e) {}
  }



  D_resize::D_resize()
  {}

  D_resize::D_resize(std::string f, double s) :
    _filter(f), _support(s)
  {}

  //! Read a D_resize record from a YAML file
  void operator >> (const YAML::Node& node, D_resize& dr) {
    try {
      node["filter"] >> dr._filter;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["support"] >> dr._support;
    } catch(YAML::RepresentationException& e) {}
  }



  D_target::D_target(std::string n, double w, double h) :
    _name(n),
    _width(w), _height(h)
  {}

  D_target::D_target(std::string &n) :
    _name(n)
  {}

  //! Read a D_target record from a YAML file
  void operator >> (const YAML::Node& node, D_target& dt) {
    try {
      node["width"] >> dt._width;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["height"] >> dt._height;
    } catch(YAML::RepresentationException& e) {}
  }



  D_JPEG::D_JPEG()
  {}

  D_JPEG::D_JPEG(int q, char h, char v, bool p) :
    _quality(q),
    _sample(std::pair<char, char>(h, v)),
    _progressive(p)
  {}

  void D_JPEG::add_variables(hash& vars) {
    hash::iterator vi;
    if ((vi = vars.find("qual")) != vars.end()) {
      _quality = atoi(vi->second.c_str());
      vars.erase(vi);
    }
    if ((vi = vars.find("sample")) != vars.end()) {
      char h, v;
      int rc = sscanf(vi->second.c_str(), "%hhdx%hhd", &h, &v);
      if (rc == 2) {
	_sample = std::pair<char, char>(h, v);
	vars.erase(vi);
      } else
	std::cerr << "D_JPEG: Failed to parse sample \"" << vi->second << "\"." << std::endl;
    }

    // If any one of the parameters are defined, then the whole object is 'defined'
    if (_quality.defined()
	|| _sample.defined()
	|| _progressive.defined())
      set_defined();
  }

  //! Read a D_JPEG record from a YAML file
  void operator >> (const YAML::Node& node, D_JPEG& dj) {
    try {
      node["qual"] >> dj._quality;
    } catch(YAML::RepresentationException& e) {}
    try {
      std::string sample;
      node["sample"] >> sample;
      char h, v;
      int rc = sscanf(sample.c_str(), "%hhdx%hhd", &h, &v);
      if (rc == 2)
	dj._sample = std::pair<char, char>(h, v);
      else
	std::cerr << "D_JPEG: Failed to parse sample \"" << sample << "\"." << std::endl;
    } catch(YAML::RepresentationException& e) {}
    try {
      int pro;
      node["pro"] >> pro;
      dj._progressive = pro > 0 ? true : false;
    } catch(YAML::RepresentationException& e) {}
  }



  D_PNG::D_PNG() {
  }

  //! Read a D_PNG record from a YAML file
  void operator >> (const YAML::Node& node, D_PNG& dp) {
  }



  D_profile::D_profile()
  {}

  //! Read a D_profile record from a YAML file
  void operator >> (const YAML::Node& node, D_profile& dp) {
    try {
      node["name"] >> dp._name;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["filename"] >> dp._filepath;
    } catch(YAML::RepresentationException& e) {}
  }


  D_thumbnail::D_thumbnail()
  {}

  //! Read a D_thumbnail record from a YAML file
  void operator >> (const YAML::Node& node, D_thumbnail& dt) {
    try {
      node["generate"] >> dt._generate;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["maxwidth"] >> dt._maxwidth;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["maxheight"] >> dt._maxheight;
    } catch(YAML::RepresentationException& e) {}
  }



  Destination::Destination()
  {}

  Destination::Destination(const Destination& other) :
    _name(other._name), _dir(other._dir),
    _size(other._size),
    _sharpen(other._sharpen), _resize(other._resize),
    _format(other._format),
    _depth(other._depth),
    _noresize(other._noresize),
    _jpeg(other._jpeg), _png(other._png),
    _intent(other._intent),
    _profile(other._profile),
    _forcergb(other._forcergb),
    _thumbnail(other._thumbnail),
    _variables(other._variables)
  {
    for (std::map<std::string, D_target::ptr>::const_iterator ti = other._targets.begin(); ti != other._targets.end(); ti++)
      _targets.insert(std::pair<std::string, D_target::ptr>(ti->first, D_target::ptr(new D_target(*(ti->second)))));
  }

  Destination::~Destination() {
  }

  Destination& Destination::operator=(const Destination& b) {
    if (this != &b) {
      _name = b._name;
      _dir = b._dir;
      _size = b._size;
      _sharpen = b._sharpen;
      _resize = b._resize;
      _format = b._format;
      _depth = b._depth;
      _noresize = b._noresize;
      _jpeg = b._jpeg;
      _png = b._png;
      _intent = b._intent;
      _profile = b._profile;
      _forcergb = b._forcergb;
      _thumbnail = b._thumbnail;
      _variables = b._variables;

      for (std::map<std::string, D_target::ptr>::const_iterator ti = b._targets.begin(); ti != b._targets.end(); ti++)
	_targets.insert(std::pair<std::string, D_target::ptr>(ti->first, D_target::ptr(new D_target(*(ti->second)))));
    }

    return *this;
  }

  Destination::ptr Destination::add_variables(hash& vars) {
    Destination::ptr ret = Destination::ptr(new Destination(*this));
    ret->_jpeg.add_variables(vars);
    ret->_variables = vars;

    return ret;
  }

  Frame::ptr Destination::best_frame(Image::ptr img) {
    if (_targets.size() == 0)
      throw NoTargets(_name);

    std::cerr << "Finding best target from \"" << _name << "\" to fit image " << img->width() << "x" << img->height() << "..." << std::endl;
    Frame::ptr best_frame;
    double best_waste = 0;
    for (std::map<std::string, D_target::ptr>::const_iterator ti = _targets.begin(); ti != _targets.end(); ti++) {
      D_target::ptr target = ti->second;
      double waste;
      double x, y;
      double width, height;
      hash::iterator vi;

      if (target->width() * img->height() > target->height() * img->width()) {
	width = img->width();
	height = img->width() * target->height() / target->width();
	x = 0;

	double offy = 0.5;
	if ((vi = _variables.find("offy")) != _variables.end())
	  offy = atof(vi->second.c_str());

	double gap = waste = img->height() - height;
	y = gap * offy / 100;
      } else {
	height = img->height();
	width = img->height() * target->width() / target->height();
	y = 0;

	double offx = 0.5;
	if ((vi = _variables.find("offx")) != _variables.end())
	  offx = atof(vi->second.c_str());

	double gap = waste = img->width() - width;
	x = gap * offx / 100;
      }
      std::cerr << "\tWaste from target \"" << target->name() << "\" ("
		<< std::setprecision(1) << std::fixed << x << ", " << y << ") + ("
		<< width << "×" << height << ") = "
		<< std::setprecision(2) << std::fixed << waste << "." << std::endl;
      if ((!best_frame) || (waste < best_waste)) {
	Frame::ptr new_best_frame(new Frame(*target, x, y, width, height, 0));
	best_frame.swap(new_best_frame);
	best_waste = waste;
      }
    }

    if (!best_frame)
      throw NoResults("Destination", "best_frame");

    std::cerr << "Least waste was from frame \"" << best_frame->name() << "\" = " << best_waste << "." << std::endl;
    return best_frame;
  }

  void operator >> (const YAML::Node& node, Destination& d) {
    try {
      node["name"] >> d._name;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["dir"] >> d._dir;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["size"] >> d._size;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["format"] >> d._format;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["depth"] >> d._depth;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["noresize"] >> d._noresize;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["forcergb"] >> d._forcergb;
    } catch(YAML::RepresentationException& e) {}
    try {
      std::string intent;
      node["intent"] >> intent;
      if (strncasecmp(intent.c_str(), "perceptual", 10) == 0)
	d._intent = INTENT_PERCEPTUAL;
      else if (strncasecmp(intent.c_str(), "relative", 8) == 0)
	d._intent = INTENT_RELATIVE_COLORIMETRIC;
      else if (strncasecmp(intent.c_str(), "saturation", 10) == 0)
	d._intent = INTENT_SATURATION;
      else if (strncasecmp(intent.c_str(), "absolute", 8) == 0)
	d._intent = INTENT_ABSOLUTE_COLORIMETRIC;
      else
	d._intent = INTENT_PERCEPTUAL;
    } catch(YAML::RepresentationException& e) {}

    if (const YAML::Node *sharpen = node.FindValue("sharpen")) {
      *sharpen >> d._sharpen;
    }

    if (const YAML::Node *resize = node.FindValue("resize")) {
      *resize >> d._resize;
    }

    if (const YAML::Node *png = node.FindValue("png")) {
      *png >> d._png;
    }

    if (const YAML::Node *jpeg = node.FindValue("jpeg")) {
      *jpeg >> d._jpeg;
    }

    if (const YAML::Node *profile = node.FindValue("profile")) {
      *profile >> d._profile;
    }

    if (const YAML::Node *thumbnail = node.FindValue("thumbnail")) {
      *thumbnail >> d._thumbnail;
    }

    if (const YAML::Node *targets = node.FindValue("targets")) {
      for(YAML::Iterator ti = targets->begin(); ti != targets->end(); ti++) {
	std::string name;
	ti.first() >> name;
	D_target::ptr target = D_target::ptr(new D_target(name));
	ti.second() >> *target;
	d._targets[name] = target;
      }
    }
  }


  Destinations::Destinations(fs::path filepath) {
    Load(filepath);
  }

  Destinations::Destinations(const Destinations& other) {
    for (const_iterator di = other._destinations.begin(); di != other._destinations.end(); di++)
      _destinations.insert(std::pair<std::string, Destination::ptr>(di->first, Destination::ptr(new Destination(*(di->second)))));
  }

  Destinations::~Destinations() {
  }

  Destinations& Destinations::operator=(const Destinations& b) {
    if (this != &b) {
      for (const_iterator di = b._destinations.begin(); di != b._destinations.end(); di++)
	_destinations.insert(std::pair<std::string, Destination::ptr>(di->first, Destination::ptr(new Destination(*(di->second)))));
    }

    return *this;
  }

  void Destinations::Load(fs::path filepath) {
    std::ifstream fin(filepath.native());
    if (fin.fail())
      throw FileOpenError(filepath.native());

    YAML::Parser parser(fin);
    YAML::Node doc;

    parser.GetNextDocument(doc);
    for (YAML::Iterator it = doc.begin(); it != doc.end(); it++) {
      std::string destname;
      it.first() >> destname;
      Destination::ptr destination = Destination::ptr(new Destination);
      it.second() >> *destination;
      _destinations[destname] = destination;
    }
  }

}
