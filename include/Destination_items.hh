#ifndef __DESTINATION_ITEMS_HH__
#define __DESTINATION_ITEMS_HH__

#include "yaml-cpp/yaml.h"
#include <string>
#include <lcms2.h>
#include "Image.hh"

namespace PhotoFinish {

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
    std::string _filter;
    bool _has_support;
    double _support;

  public:
    D_resize();
    D_resize(const D_resize& other);
    ~D_resize();

    inline bool has_filter(void) const { return _has_filter; }
    inline std::string filter(void) const { return _filter; }
    inline bool has_support(void) const { return _has_support; }
    inline double support(void) const { return _support; }

    friend void operator >> (const YAML::Node& node, D_resize& dr);
  };

  class D_target {
  protected:
    std::string _name;
    double _width, _height;

  public:
    D_target(std::string &n);
    D_target(const D_target& other);
    ~D_target();

    inline std::string name(void) const { return _name; }
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
    std::string _name, _filepath;
    cmsHPROFILE _profile;

  public:
    D_profile();
    D_profile(const D_profile& other);
    ~D_profile();

    inline std::string name(void) const { return _name; }
    inline std::string filepath(void) const { return _filepath; }
    inline bool has_profle(void) const { return _profile != NULL; }
    inline cmsHPROFILE profile(void) const { return _profile; }

    friend void operator >> (const YAML::Node& node, D_profile& dp);
  };

}

#endif // __DESTINATION_ITEMS_HH__
