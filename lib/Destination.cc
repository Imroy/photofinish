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
#include <fstream>
#include <strings.h>
#include <string.h>
#include <math.h>
#include "Destination_items.hh"
#include "Destination.hh"

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

  D_sharpen::D_sharpen() :
    _has_radius(false),
    _has_sigma(false),
    _radius(-1),
    _sigma(-1)
  {}

  void operator >> (const YAML::Node& node, D_sharpen& ds) {
    try {
      node["radius"] >> ds._radius;
      ds._has_radius = true;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["sigma"] >> ds._sigma;
      ds._has_sigma = true;
    } catch(YAML::RepresentationException& e) {}
  }



  D_resize::D_resize() :
    _has_filter(false),
    _has_support(false),
    _filter("Lanczos"),
    _support(3)
  {}

  void operator >> (const YAML::Node& node, D_resize& dr) {
    try {
      node["filter"] >> dr._filter;
      dr._has_filter = true;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["support"] >> dr._support;
      dr._has_support = true;
    } catch(YAML::RepresentationException& e) {}
  }



  D_target::D_target(std::string &n) :
    _has_width(false), _has_height(false),
    _name(n),
    _width(-1), _height(1)
  {}

  void operator >> (const YAML::Node& node, D_target& dt) {
    try {
      node["width"] >> dt._width;
      dt._has_width = true;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["height"] >> dt._height;
      dt._has_height = true;
    } catch(YAML::RepresentationException& e) {}
  }



  D_JPEG::D_JPEG() :
    _has_quality(false),
    _has_sample(false),
    _has_progressive(false),
    _quality(90),
    _sample_h(2), _sample_v(2),
    _progressive(true)
  {}

  bool D_JPEG::add_variables(hash& vars) {
    bool ret = false;
    hash::iterator vi;
    if ((vi = vars.find("qual")) != vars.end()) {
      _quality = atoi(vi->second.c_str());
      _has_quality = true;
      vars.erase(vi);
      ret = true;
    }
    if ((vi = vars.find("sample")) != vars.end()) {
      int rc = sscanf(vi->second.c_str(), "%hhdx%hhd", &_sample_h, &_sample_v);
      if (rc < 2)
	fprintf(stderr, "D_JPEG: Failed to parse sample \"%s\".\n", vi->second.c_str());
      else {
	_has_sample = true;
	vars.erase(vi);
	ret = true;
      }
    }
    return ret;
  }

  void operator >> (const YAML::Node& node, D_JPEG& dj) {
    try {
      node["qual"] >> dj._quality;
      dj._has_quality = true;
    } catch(YAML::RepresentationException& e) {}
    try {
      std::string sample;
      node["sample"] >> sample;
      int rc = sscanf(sample.c_str(), "%hhdx%hhd", &dj._sample_h, &dj._sample_v);
      if (rc < 2)
	fprintf(stderr, "D_JPEG: Failed to parse sample \"%s\".\n", sample.c_str());
      else
	dj._has_sample = true;
    } catch(YAML::RepresentationException& e) {}
    try {
      int pro;
      node["pro"] >> pro;
      dj._progressive = pro > 0 ? true : false;
      dj._has_progressive = true;
    } catch(YAML::RepresentationException& e) {}
  }



  D_PNG::D_PNG() {
  }

  void operator >> (const YAML::Node& node, D_PNG& dp) {
  }



  D_profile::D_profile() :
    _has_name(false),
    _has_filepath(false)
  {}

  void operator >> (const YAML::Node& node, D_profile& dp) {
    try {
      node["name"] >> dp._name;
      dp._has_name = true;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["filename"] >> dp._filepath;
      dp._has_filepath = true;
    } catch(YAML::RepresentationException& e) {}
  }



  Destination::Destination() :
    _has_name(false), _has_dir(false),
    _has_size(false),
    _size(-1),
    _has_sharpen(false), _has_resize(false),
    _has_format(false), _format("JPEG"),
    _has_depth(false), _depth(8),
    _has_noresize(false), _noresize(false),
    _has_jpeg(false), _has_png(false),
    _has_intent(false), _intent(INTENT_PERCEPTUAL),
    _has_profile(false),
    _has_forcergb(false), _forcergb(false)
  {}

  Destination::Destination(const Destination& other) :
    _has_name(other._has_name), _has_dir(other._has_dir),
    _name(other._name), _dir(other._dir),
    _has_size(other._has_size), _size(other._size),
    _has_sharpen(other._has_sharpen), _has_resize(other._has_resize),
    _sharpen(other._sharpen), _resize(other._resize),
    _has_format(other._has_format), _format(other._format),
    _has_depth(other._has_depth), _depth(other._depth),
    _has_noresize(other._has_noresize), _noresize(other._noresize),
    _has_jpeg(other._has_jpeg), _has_png(other._has_png),
    _jpeg(other._jpeg), _png(other._png),
    _has_intent(other._has_intent), _intent(other._intent),
    _has_profile(other._has_profile), _profile(other._profile),
    _has_forcergb(other._has_forcergb), _forcergb(other._forcergb),
    _variables(other._variables)
  {
    for (std::map<std::string, D_target::ptr>::const_iterator ti = other._targets.begin(); ti != other._targets.end(); ti++)
      _targets.insert(std::pair<std::string, D_target::ptr>(ti->first, D_target::ptr(new D_target(*(ti->second)))));
  }

  Destination::~Destination() {
  }

  Destination& Destination::operator=(const Destination& b) {
    if (this != &b) {
      _has_name = b._has_name;
      _has_dir = b._has_dir;
      _name = b._name;
      _dir = b._dir;
      _has_size = b._has_size;
      _size = b._size;
      _has_sharpen = b._has_sharpen;
      _has_resize = b._has_resize;
      _sharpen = b._sharpen;
      _resize = b._resize;
      _has_format = b._has_format;
      _format = b._format;
      _has_depth = b._has_depth;
      _depth = b._depth;
      _has_noresize = b._has_noresize;
      _noresize = b._noresize;
      _has_jpeg = b._has_jpeg;
      _has_png = b._has_png;
      _jpeg = b._jpeg;
      _png = b._png;
      _has_intent = b._has_intent;
      _intent = b._intent;
      _has_profile = b._has_profile;
      _profile = b._profile;
      _has_forcergb = b._has_forcergb;
      _forcergb = b._forcergb;
      _variables = b._variables;

      for (std::map<std::string, D_target::ptr>::const_iterator ti = b._targets.begin(); ti != b._targets.end(); ti++)
	_targets.insert(std::pair<std::string, D_target::ptr>(ti->first, D_target::ptr(new D_target(*(ti->second)))));
    }

    return *this;
  }

  Destination::ptr Destination::add_variables(hash& vars) {
    Destination::ptr ret = Destination::ptr(new Destination(*this));
    if (ret->_jpeg.add_variables(vars))
      ret->_has_jpeg = true;
    ret->_variables = vars;

    return ret;
  }

  Frame::ptr Destination::best_frame(Image::ptr img) {
    if (_targets.size() == 0)
      throw NoTargets(_name);

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
      fprintf(stderr, "Waste from target \"%s\" (%0.1f,%0.1f)+(%0.1fx%0.1f) = %0.2f.\n", target->name().c_str(), x, y, width, height, waste);
      if ((!best_frame) || (waste < best_waste)) {
	Frame::ptr new_best_frame(new Frame(*target, x, y, width, height, 0));
	best_frame.swap(new_best_frame);
	best_waste = waste;
      }
    }

    if (!best_frame)
      throw NoResults("Destination", "best_frame");

    fprintf(stderr, "Best waste was from frame \"%s\" = %f.\n", best_frame->name().c_str(), best_waste);
    return best_frame;
  }

  void operator >> (const YAML::Node& node, Destination& d) {
    try {
      node["name"] >> d._name;
      d._has_name = true;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["dir"] >> d._dir;
      d._has_dir = true;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["size"] >> d._size;
      d._has_size = true;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["format"] >> d._format;
      d._has_format = true;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["depth"] >> d._depth;
      d._has_depth = true;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["noresize"] >> d._noresize;
      d._has_noresize = true;
    } catch(YAML::RepresentationException& e) {}
    try {
      node["forcergb"] >> d._forcergb;
      d._has_forcergb = true;
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
      d._has_intent = true;
    } catch(YAML::RepresentationException& e) {}

    if (const YAML::Node *sharpen = node.FindValue("sharpen")) {
      *sharpen >> d._sharpen;
      d._has_sharpen = true;
    }

    if (const YAML::Node *resize = node.FindValue("resize")) {
      *resize >> d._resize;
      d._has_resize = true;
    }

    if (const YAML::Node *png = node.FindValue("png")) {
      *png >> d._png;
      d._has_png = true;
    }

    if (const YAML::Node *jpeg = node.FindValue("jpeg")) {
      *jpeg >> d._jpeg;
      d._has_jpeg = true;
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
