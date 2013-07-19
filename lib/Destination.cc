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
#include <memory>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <string.h>
#include "Destination_items.hh"
#include "Destination.hh"
#include "CropSolution.hh"
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
    _tiff(other._tiff), _jp2(other._jp2), _webp(other._webp),
    _intent(other._intent),
    _profile(other._profile),
    _forcergb(other._forcergb),
    _forcegrey(other._forcegrey),
    _thumbnail(other._thumbnail),
    _variables(other._variables)
  {
    for (auto ti : other._targets)
      _targets.insert(std::pair<std::string, D_target::ptr>(ti.first, std::make_shared<D_target>(*(ti.second))));
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
      _webp = b._webp;
      _intent = b._intent;
      _profile = b._profile;
      _forcergb = b._forcergb;
      _forcegrey = b._forcegrey;
      _thumbnail = b._thumbnail;
      _variables = b._variables;

      for (auto ti : b._targets)
	_targets.insert(std::pair<std::string, D_target::ptr>(ti.first, std::make_shared<D_target>(*(ti.second))));
    }

    return *this;
  }

  Destination::ptr Destination::add_variables(multihash& vars) {
    auto ret = std::make_shared<Destination>(*this);

    ImageFile::add_variables(ret, vars);
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
    for (auto ti : _targets) {
      auto target = ti.second;
      std::cerr << "\tTarget \"" << target->name() << "\" (" << target->width() << "×" << target->height() << "):" << std::endl;

      if ((target->width() > img->width()) && (target->height() > img->height())) {
	std::cerr << "\tSkipping because the target is larger than the original image in both dimensions." << std::endl;
	continue;
      }

      if (target->width() * target->height() > img->width() * img->height()) {
	std::cerr << "\tSkipping because the target has more pixels than the original image." << std::endl;
	continue;
      }

      std::cerr << std::setprecision(2) << std::fixed;
      auto frame = solver.solve(img, target);

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
		<< frame->crop_x() << ", " << frame->crop_y() << ") + ("
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

  CMS::Format Destination::modify_format(CMS::Format format) {
    if (this->forcergb().defined() && this->forcergb()) {
      std::cerr << "Forcing RGB..." << std::endl;
      format.set_colour_model(CMS::ColourModel::RGB);
      format.set_channels(3);
    }

    if (this->forcegrey().defined() && this->forcegrey()) {
      std::cerr << "Forcing greyscale..." << std::endl;
      format.set_colour_model(CMS::ColourModel::Greyscale);
      format.set_channels(1);
    }

    if (this->depth().defined()) {
      std::cerr << "Changing depth to " << (this->depth() >> 3) << " bytes..." << std::endl;
      switch (this->depth() >> 3) {
      case 1: format.set_8bit();
	break;

      case 2: format.set_16bit();
	break;

      case 4: format.set_32bit();
	break;

      default:
	std::cerr << "Invalid depth." << std::endl;
	format.set_8bit();
	break;
      }
    } else {
      std::cerr << "Changing depth to 1 byte..." << std::endl;
      format.set_8bit();
    }

    return format;
  }

  CMS::Profile::ptr Destination::get_profile(CMS::ColourModel default_colourmodel, std::string for_desc) {
    CMS::Profile::ptr profile;

    if (this->profile()) {
      profile = this->profile()->profile();

      std::string profile_name = this->profile()->name();
      std::cerr << "Adding name \"" << profile_name << "\" to profile..." << std::endl;
      profile->write_tag(cmsSigProfileDescriptionTag, "en", "AU", profile_name);
    } else {
      profile = Image::default_profile(default_colourmodel, for_desc);
    }

    return profile;
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
	d._intent = CMS::Intent::Perceptual;
      else if (boost::iequals(intent.substr(0, 8), "relative"))
	d._intent = CMS::Intent::Relative_colormetric;
      else if (boost::iequals(intent.substr(0, 10), "saturation"))
	d._intent = CMS::Intent::Saturation;
      else if (boost::iequals(intent.substr(0, 8), "absolute"))
	d._intent = CMS::Intent::Absolute_colormetric;
      else
	d._intent = CMS::Intent::Perceptual;
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

    if (const YAML::Node *n = node.FindValue("webp"))
      *n >> d._webp;

    if (const YAML::Node *n = node.FindValue("profile")) {
      d._profile = std::make_shared<D_profile>();
      *n >> *(d._profile);
    }

    if (const YAML::Node *n = node.FindValue("thumbnail"))
      *n >> d._thumbnail;

    if (const YAML::Node *targets = node.FindValue("targets")) {
      for(auto ti = targets->begin(); ti != targets->end(); ti++) {
	std::string name;
	ti.first() >> name;
	auto target = std::make_shared<D_target>(name);
	ti.second() >> *target;
	d._targets[name] = target;
      }
    }
  }


  Destinations::Destinations(fs::path filepath) {
    Load(filepath);
  }

  Destinations::Destinations(const Destinations& other) {
    for (auto di : other._destinations)
      _destinations.insert(std::pair<std::string, Destination::ptr>(di.first, std::make_shared<Destination>(*(di.second))));
  }

  Destinations::~Destinations() {
  }

  Destinations& Destinations::operator=(const Destinations& b) {
    if (this != &b) {
      for (auto di : b._destinations)
	_destinations.insert(std::pair<std::string, Destination::ptr>(di.first, std::make_shared<Destination>(*(di.second))));
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
    for (auto it = doc.begin(); it != doc.end(); it++) {
      std::string destname;
      it.first() >> destname;
      auto destination = std::make_shared<Destination>();
      it.second() >> *destination;
      _destinations[destname] = destination;
    }
  }

}
