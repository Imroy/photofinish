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
#include "Destination_items.hh"
#include "Destination.hh"
#include "CropSolution.hh"
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
    if (const YAML::Node *n = node.FindValue("radius"))
      *n >> ds._radius;

    if (const YAML::Node *n = node.FindValue("sigma"))
      *n >> ds._sigma;

    ds.set_defined();
  }



  D_resize::D_resize()
  {}

  D_resize::D_resize(std::string f, double s) :
    _filter(f), _support(s)
  {}

  //! Read a D_resize record from a YAML file
  void operator >> (const YAML::Node& node, D_resize& dr) {
    if (const YAML::Node *n = node.FindValue("filter"))
      *n >> dr._filter;

    if (const YAML::Node *n = node.FindValue("support"))
      *n >> dr._support;

    dr.set_defined();
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
    if (const YAML::Node *n = node.FindValue("width"))
      *n >> dt._width;

    if (const YAML::Node *n = node.FindValue("height"))
      *n >> dt._height;

    if (const YAML::Node *n = node.FindValue("size"))
      *n >> dt._size;
  }



  D_JPEG::D_JPEG()
  {}

  D_JPEG::D_JPEG(int q, char h, char v, bool p) :
    _quality(q),
    _sample(std::pair<int, int>(h, v)),
    _progressive(p)
  {}

  void D_JPEG::add_variables(multihash& vars) {
    multihash::iterator vi;
    if (!_quality.defined() && ((vi = vars.find("qual")) != vars.end())) {
      _quality = boost::lexical_cast<int>(vi->second[0]);
      vars.erase(vi);
      set_defined();
    }
    if (!_sample.defined() && ((vi = vars.find("sample")) != vars.end())) {
      std::string sample = vi->second[0];
      size_t x = sample.find_first_of("x×");
      if (x != std::string::npos) {
	try {
	  int h = boost::lexical_cast<int>(sample.substr(0, x));
	  int v = boost::lexical_cast<int>(sample.substr(x + 1, sample.length() - x - 1));
	  _sample = std::pair<int, int>(h, v);
	  vars.erase(vi);
	  set_defined();
	} catch (boost::bad_lexical_cast &ex) {
	  std::cerr << ex.what();
	}
      } else
	std::cerr << "D_JPEG: Failed to parse sample \"" << vi->second[0] << "\"." << std::endl;
    }
  }

  //! Read a D_JPEG record from a YAML file
  void operator >> (const YAML::Node& node, D_JPEG& dj) {
    if (const YAML::Node *n = node.FindValue("qual"))
      *n >> dj._quality;

    if (const YAML::Node *n = node.FindValue("sample")) {
      std::string sample;
      *n >> sample;
      size_t x = sample.find_first_of("x×");
      if (x != std::string::npos) {
	try {
	  int h = boost::lexical_cast<int>(sample.substr(0, x));
	  int v = boost::lexical_cast<int>(sample.substr(x + 1, sample.length() - x - 1));
	  dj._sample = std::pair<int, int>(h, v);
	} catch (boost::bad_lexical_cast &ex) {
	  std::cerr << ex.what();
	}
      } else
	std::cerr << "D_JPEG: Failed to parse sample \"" << sample << "\"." << std::endl;
    }
    if (const YAML::Node *n = node.FindValue("pro"))
      *n >> dj._progressive;

    dj.set_defined();
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
    if (const YAML::Node *n = node.FindValue("name"))
      *n >> dp._name;

    if (const YAML::Node *n = node.FindValue("filename"))
      *n >> dp._filepath;

    dp.set_defined();
  }


  D_thumbnail::D_thumbnail()
  {}

  //! Read a D_thumbnail record from a YAML file
  void operator >> (const YAML::Node& node, D_thumbnail& dt) {
    if (const YAML::Node *n = node.FindValue("generate"))
      *n >> dt._generate;

    if (const YAML::Node *n = node.FindValue("maxwidth"))
      *n >> dt._maxwidth;

    if (const YAML::Node *n = node.FindValue("maxheight"))
      *n >> dt._maxheight;

    dt.set_defined();
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
    ret->_jpeg.add_variables(vars);
    ret->_variables = vars;

    return ret;
  }

  Frame::ptr Destination::best_frame(Image::ptr img) {
    if (_targets.size() == 0)
      throw NoTargets(_name);

    std::cerr << "Finding best target from \"" << _name << "\" to fit image " << img->width() << "x" << img->height() << "..." << std::endl;
    CropSolver solver(_variables);
    Frame::ptr best_frame;
    double best_waste = 0;
    for (std::map<std::string, D_target::ptr>::const_iterator ti = _targets.begin(); ti != _targets.end(); ti++) {
      D_target::ptr target = ti->second;
      std::cerr << "\tTarget \"" << target->name() << "\" (" << target->width() << "×" << target->height() << "):" << std::endl;

      if ((target->width() > img->width()) && (target->height() > img->height())) {
	std::cerr << "\tSkipping because the target is larger than the original image in both dimensions." << std::endl;
	continue;
      }

      if (target->width() * target->height() > img->width() * img->height()) {
	std::cerr << "\tSkipping because the target has more pixels than the original image." << std::endl;
	continue;
      }

      Frame::ptr frame = solver.solve(img, target);

      if ((target->width() > frame->crop_w()) && (target->height() > frame->crop_h())) {
	std::cerr << "\tSkipping because the target is larger than the cropped image in both dimensions." << std::endl;
	continue;
      }

      if (target->width() * target->height() > frame->crop_w() * frame->crop_h()) {
	std::cerr << "\tSkipping because the target has more pixels than the cropped image." << std::endl;
	continue;
      }

      double waste = frame->waste(img);

      std::cerr << "\tWaste from ("
		<< std::setprecision(2) << std::fixed << frame->crop_x() << ", " << frame->crop_y() << ") + ("
		<< frame->crop_w() << "×" << frame->crop_h() << ") = "
		<< waste << "." << std::endl;
      if ((!best_frame) || (waste < best_waste)) {
	best_frame.swap(frame);
	best_waste = waste;
      }
    }

    if (!best_frame)
      throw NoResults("Destination", "best_frame");

    std::cerr << "Least waste was from frame \"" << best_frame->name() << "\" = " << best_waste << "." << std::endl;
    return best_frame;
  }

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

    if (const YAML::Node *n = node.FindValue("png"))
      *n >> d._png;

    if (const YAML::Node *n = node.FindValue("jpeg"))
      *n >> d._jpeg;

    if (const YAML::Node *n = node.FindValue("profile"))
      *n >> d._profile;

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
