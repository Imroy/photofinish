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
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <string.h>
#include "Destination_items.hh"
#include "Destination.hh"
#include "ImageFile.hh"
#include "Exception.hh"

namespace PhotoFinish {

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
    _tiff(other._tiff), _jp2(other._jp2),
    _intent(other._intent),
    _profile(other._profile),
    _forcergb(other._forcergb),
    _forcegrey(other._forcegrey),
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
      _tiff = b._tiff;
      _jp2 = b._jp2;
      _intent = b._intent;
      _profile = b._profile;
      _forcergb = b._forcergb;
      _forcegrey = b._forcegrey;
      _thumbnail = b._thumbnail;
      _variables = b._variables;

      for (std::map<std::string, D_target::ptr>::const_iterator ti = b._targets.begin(); ti != b._targets.end(); ti++)
	_targets.insert(std::pair<std::string, D_target::ptr>(ti->first, D_target::ptr(new D_target(*(ti->second)))));
    }

    return *this;
  }

  Destination::ptr Destination::add_variables(multihash& vars) {
    Destination::ptr ret = Destination::ptr(new Destination(*this));

    add_format_variables(ret, vars);
    ret->_variables = vars;

    return ret;
  }

  //! Read a Destination record from a YAML file
  void operator >> (const YAML::Node& node, Destination& d) {
    if (const YAML::Node *n = node.FindValue("name"))
      *n >> d._name;

    if (const YAML::Node *n = node.FindValue("dir"))
      *n >> d._dir;

    if (const YAML::Node *n = node.FindValue("size"))
      *n >> d._size;

    if (const YAML::Node *n = node.FindValue("format"))
      *n >> d._format;

    if (const YAML::Node *n = node.FindValue("depth"))
      *n >> d._depth;

    if (const YAML::Node *n = node.FindValue("noresize"))
      *n >> d._noresize;

    if (const YAML::Node *n = node.FindValue("forcergb"))
      *n >> d._forcergb;

    if (const YAML::Node *n = node.FindValue("forcegrey"))
      *n >> d._forcegrey;

    if (const YAML::Node *n = node.FindValue("intent")) {
      std::string intent;
      *n >> intent;
      if (boost::iequals(intent.substr(0, 10), "perceptual"))
	d._intent = INTENT_PERCEPTUAL;
      else if (boost::iequals(intent.substr(0, 8), "relative"))
	d._intent = INTENT_RELATIVE_COLORIMETRIC;
      else if (boost::iequals(intent.substr(0, 10), "saturation"))
	d._intent = INTENT_SATURATION;
      else if (boost::iequals(intent.substr(0, 8), "absolute"))
	d._intent = INTENT_ABSOLUTE_COLORIMETRIC;
      else
	d._intent = INTENT_PERCEPTUAL;
    }

    if (const YAML::Node *n = node.FindValue("sharpen"))
      *n >> d._sharpen;

    if (const YAML::Node *n = node.FindValue("resize"))
      *n >> d._resize;

    if (const YAML::Node *n = node.FindValue("jpeg"))
      *n >> d._jpeg;

    if (const YAML::Node *n = node.FindValue("png"))
      *n >> d._png;

    if (const YAML::Node *n = node.FindValue("tiff"))
      *n >> d._tiff;

    if (const YAML::Node *n = node.FindValue("jp2"))
      *n >> d._jp2;

    if (const YAML::Node *n = node.FindValue("profile")) {
      d._profile = D_profile::ptr(new D_profile);
      *n >> *(d._profile);
    }

    if (const YAML::Node *n = node.FindValue("thumbnail"))
      *n >> d._thumbnail;

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
