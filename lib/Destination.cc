#include <strings.h>
#include <string.h>
#include <math.h>
#include "Destination_items.hh"
#include "Destination.hh"

namespace PhotoFinish {

  D_sharpen::D_sharpen() :
    _has_radius(false),
    _has_sigma(false),
    _radius(-1),
    _sigma(-1)
  {}

  D_sharpen::D_sharpen(const D_sharpen& other) :
    _has_radius(other._has_radius),
    _has_sigma(other._has_sigma),
    _radius(other._radius),
    _sigma(other._sigma)
  {}

  D_sharpen::~D_sharpen() {
  }

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

  D_resize::D_resize(const D_resize& other) :
    _has_filter(other._has_filter),
    _has_support(other._has_support),
    _filter(other._filter),
    _support(other._support)
  {}

  D_resize::~D_resize() {
  }

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

  D_target::D_target(const D_target& other) :
    _has_width(other._has_width),
    _has_height(other._has_height),
    _name(other._name),
    _width(other._width),
    _height(other._height)
  {}

  D_target::~D_target() {
  }

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

  D_JPEG::D_JPEG(const D_JPEG& other) :
    _has_quality(other._has_quality),
    _has_sample(other._has_sample),
    _has_progressive(other._has_progressive),
    _quality(other._quality),
    _sample_h(other._sample_h),
    _sample_v(other._sample_v),
    _progressive(other._progressive)
  {}

  D_JPEG::~D_JPEG() {
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

  D_PNG::D_PNG(const D_PNG& other)
  {}

  D_PNG::~D_PNG() {
  }

  void operator >> (const YAML::Node& node, D_PNG& dp) {
  }



  D_profile::D_profile() :
    _has_name(false),
    _has_filepath(false)
  {}

  D_profile::D_profile(const D_profile& other) :
    _has_name(other._has_name),
    _has_filepath(other._has_filepath),
    _name(other._name),
    _filepath(other._filepath)
  {}

  D_profile::~D_profile() {
  }

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
    _has_profile(false)
  {}

  Destination::Destination(const Destination& other) :
    _has_name(other._has_name), _has_dir(other._has_dir),
    _name(other._name), _dir(other._dir),
    _has_size(other._has_size), _size(other._size),
    _has_sharpen(other._has_sharpen), _has_resize(other._has_resize),
    _sharpen(other._sharpen), _resize(other._resize),
    _targets(other._targets),
    _has_format(other._has_format), _format(other._format),
    _has_depth(other._has_depth), _depth(other._depth),
    _has_noresize(other._has_noresize), _noresize(other._noresize),
    _has_jpeg(other._has_jpeg), _has_png(other._has_png),
    _jpeg(other._jpeg), _png(other._png),
    _has_intent(other._has_intent), _intent(other._intent),
    _has_profile(other._has_profile), _profile(other._profile)
  {}

  Destination::~Destination() {
    for (std::map<std::string, D_target*>::iterator ti = _targets.begin(); ti != _targets.end(); ti++)
      delete ti->second;
  }

  const Frame& Destination::best_frame(const Image& img) {
    if (_targets.size() == 0)
      throw NoTargets(_name);

    Frame *best_frame = NULL;
    double best_waste = 0;
    for (std::map<std::string, D_target*>::const_iterator ti = _targets.begin(); ti != _targets.end(); ti++) {
      D_target *target = ti->second;
      double waste;
      double x, y;
      double width, height;

      if (target->width() * img.height() > target->height() * img.width()) {
	width = img.width();
	height = img.width() * target->height() / target->width();
	x = 0;
	double gap = waste = img.height() - height;
	y = gap * 0.5;
      } else {
	height = img.height();
	width = img.height() * target->width() / target->height();
	y = 0;
	double gap = waste = img.width() - width;
	x = gap * 0.5;
      }
      fprintf(stderr, "Waste from target \"%s\" (%0.1f,%0.1f)+(%0.1fx%0.1f) = %0.2f.\n", target->name().c_str(), x, y, width, height, waste);
      if ((best_frame == NULL) || (waste < best_waste)) {
	if (best_frame != NULL)	// delete previous best
	  delete best_frame;
	best_frame = new Frame(*target, x, y, width, height, 0);
	best_waste = waste;
      }
    }

    if (best_frame == NULL)
      throw NoResults("Destination", "best_frame");

    fprintf(stderr, "Best waste was from frame \"%s\" = %f.\n", best_frame->name().c_str(), best_waste);
    return *best_frame;
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
      int noresize;
      node["noresize"] >> noresize;
      d._noresize = noresize > 0 ? true : false;
      d._has_noresize = true;
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
	D_target *target = new D_target(name);
	ti.second() >> *target;
	d._targets[name] = target;
      }
    }
  }

}
