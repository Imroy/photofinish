#include <strings.h>
#include <string.h>
#include "Destination.hh"

D_sharpen::D_sharpen() :
  _radius(-1),
  _sigma(-1)
{}

D_sharpen::D_sharpen(const D_sharpen& other) :
  _radius(other._radius),
  _sigma(other._sigma)
{}

D_sharpen::~D_sharpen() {
}

void operator >> (const YAML::Node& node, D_sharpen& ds) {
  try {
    node["radius"] >> ds._radius;
  } catch(YAML::RepresentationException& e) {}
  try {
    node["sigma"] >> ds._sigma;
  } catch(YAML::RepresentationException& e) {}
}



D_resize::D_resize() :
  _filter("Lanczos"),
  _support(3)
{}

D_resize::D_resize(const D_resize& other) :
  _filter(other._filter),
  _support(other._support)
{}

D_resize::~D_resize() {
}

void operator >> (const YAML::Node& node, D_resize& dr) {
  try {
    node["filter"] >> dr._filter;
  } catch(YAML::RepresentationException& e) {}
  try {
    node["support"] >> dr._support;
  } catch(YAML::RepresentationException& e) {}
}



D_target::D_target(string &n) :
  _name(n)
{}

D_target::D_target(const D_target& other) :
  _name(other._name),
  _width(other._width),
  _height(other._height)
{}

D_target::~D_target() {
}

void operator >> (const YAML::Node& node, D_target& dt) {
  try {
    node["width"] >> dt._width;
  } catch(YAML::RepresentationException& e) {}
  try {
    node["height"] >> dt._height;
  } catch(YAML::RepresentationException& e) {}
}



D_JPEG::D_JPEG() :
  _quality(90),
  _sample_h(2), _sample_v(2),
  _progressive(true)
{}

D_JPEG::D_JPEG(const D_JPEG& other) :
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
  } catch(YAML::RepresentationException& e) {}
  try {
    string sample;
    node["sample"] >> sample;
    const char *sample_c = sample.c_str();
    const char *xpos = strchr(sample_c, 'x');
    if (xpos != NULL) {
      int hlen = xpos - sample_c;
      int vlen = strlen(sample_c) - hlen - 1;
      char *hstr = (char*)malloc(hlen), *vstr = (char*)malloc(vlen);

      strncpy(hstr, sample_c, hlen);
      hstr[hlen] = 0;
      dj._sample_h = atoi(hstr);
      free(hstr);

      strncpy(vstr, xpos + 1, vlen);
      vstr[vlen] = 0;
      dj._sample_v = atoi(vstr);
      free(vstr);
    }
  } catch(YAML::RepresentationException& e) {}
  try {
    int pro;
    node["pro"] >> pro;
    dj._progressive = pro > 0 ? true : false;
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
  _profile(NULL)
{}

D_profile::D_profile(const D_profile& other) :
  _name(other._name),
  _filepath(other._filepath),
  _profile(cmsOpenProfileFromFile(_filepath.c_str(), "r"))
{}

D_profile::~D_profile() {
  if (_profile != NULL) {
    cmsCloseProfile(_profile);
    _profile = NULL;
  }
}

void operator >> (const YAML::Node& node, D_profile& dp) {
  try {
    node["name"] >> dp._name;
  } catch(YAML::RepresentationException& e) {}
  try {
    node["filename"] >> dp._filepath;
    dp._profile = cmsOpenProfileFromFile(dp._filepath.c_str(), "r");
  } catch(YAML::RepresentationException& e) {}
}



Destination::Destination() :
  _size(-1),
  _has_sharpen(false), _has_resize(false),
  _format("JPEG"), _depth(8), _noresize(true),
  _has_jpeg(false), _has_png(false),
  _intent(INTENT_PERCEPTUAL),
  _has_profile(false)
{}

Destination::Destination(const Destination& other) :
  _name(other._name), _dir(other._dir),
  _size(other._size),
  _has_sharpen(other._has_sharpen), _has_resize(other._has_resize),
  _sharpen(other._sharpen), _resize(other._resize),
  _targets(other._targets),
  _format(other._format), _depth(other._depth),
  _noresize(other._noresize),
  _has_jpeg(other._has_jpeg), _has_png(other._has_png),
  _jpeg(other._jpeg), _png(other._png),
  _intent(other._intent),
  _has_profile(other._has_profile), _profile(other._profile)
{}

Destination::~Destination() {
  for (map<string, D_target*>::iterator ti = _targets.begin(); ti != _targets.end(); ti++)
    delete ti->second;
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
    int noresize;
    node["noresize"] >> noresize;
    d._noresize = noresize > 0 ? true : false;
  } catch(YAML::RepresentationException& e) {}
  try {
    string intent;
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
      string name;
      ti.first() >> name;
      D_target *target = new D_target(name);
      ti.second() >> *target;
      d._targets[name] = target;
    }
  }
}
