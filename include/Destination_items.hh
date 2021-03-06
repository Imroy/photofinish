/*
	Copyright 2014-2019 Ian Tester

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
#pragma once

#include <string>
#include <memory>
#include <vector>
#include "yaml-cpp/yaml.h"
#include <boost/filesystem.hpp>
#include "CMS.hh"
#include "Image.hh"
#include "Definable.hh"

namespace fs = boost::filesystem;

namespace PhotoFinish {

  //! A simple hash
  typedef std::map<std::string, std::string> hash;

  //! A list of strings
  typedef std::vector<std::string> stringlist;

  //! A hash of string lists
  typedef std::map<std::string, stringlist > multihash;

  //! Sharpen parameters for destination
  class D_sharpen : public Role_Definable {
  private:
    definable<double> _radius, _sigma;

  public:
    //! Empty constructor
    D_sharpen();

    inline definable<double> radius(void) const { return _radius; }
    inline definable<double> sigma(void) const { return _sigma; }

    void read_config(const YAML::Node& node);
  };

  //! Resize parameters for destination
  class D_resize : public Role_Definable {
  private:
    std::string _filter;
    definable<double> _support;

    D_resize(const std::string& f, double s);

  public:
    //! Empty constructor
    D_resize();

    //! Named constructor
    /*! Constructs a D_resize object with filter="lanczos" and the supplied radius
      \param r Radius of Lanczos filter
    */
    static inline D_resize lanczos(double r) { return D_resize("lanczos", r); }

    inline std::string filter(void) const { return _filter; }
    inline definable<double> support(void) const { return _support; }

    void read_config(const YAML::Node& node);
  };

  //! Target parameters for destination
  class D_target {
  protected:
    std::string _name;
    definable<double> _width, _height;
    definable<double> _size;		//! A target-specific size (in inches) to override the one in the destination

  public:
    D_target(const std::string& n, double w, double h);
    D_target(const std::string& n);

    inline std::string name(void) const { return _name; }
    inline definable<double> width(void) const { return _width; }
    inline definable<double> height(void) const { return _height; }
    inline definable<double> size(void) const { return _size; }

    void read_config(const YAML::Node& node);

    typedef std::shared_ptr<D_target> ptr;
  };

  //! JPEG parameters for destination
  class D_JPEG : public Role_Definable {
  private:
    definable<int> _quality;
    definable< std::pair<int, int> > _sample;
    definable<bool> _progressive;

  public:
    //! Empty constructor
    D_JPEG();

    //! Constructor
    /*!
      \param q Quality
      \param h,v Chroma sampling
      \param p Progressive
    */
    D_JPEG(int q, char h, char v, bool p);

    //! Set values from a map of "variables"
    void add_variables(multihash& vars);

    inline definable<int> quality(void) const { return _quality; }
    inline void set_quality(int q) { _quality = q; set_defined(); }

    inline definable< std::pair<int, int> > sample(void) const { return _sample; }
    inline void set_sample(int h, int v) { _sample = std::pair<int, int>(h, v); set_defined(); }

    inline definable<bool> progressive(void) const { return _progressive; }
    inline void set_progressive(bool p = true) { _progressive = p; set_defined(); }

    void read_config(const YAML::Node& node);
  };

  //! PNG parameters for destination
  class D_PNG : public Role_Definable {
  private:

  public:
    D_PNG();

    void read_config(const YAML::Node& node);
  };

  //! TIFF parameters for destination
  class D_TIFF : public Role_Definable {
  private:
    std::string _artist, _copyright;
    std::string _compression;

  public:
    //! Empty constructor
    D_TIFF();

    //! Constructor
    /*!
      \param c Compression string
    */
    D_TIFF(const std::string& c);

    //! Set values from a map of "variables"
    void add_variables(multihash& vars);

    inline std::string artist(void) const { return _artist; }
    inline void set_artist(const std::string& a) { _artist = a; set_defined(); }

    inline std::string copyright(void) const { return _copyright; }
    inline void set_copyright(const std::string& c) { _copyright = c; set_defined(); }

    inline std::string compression(void) const { return _compression; }
    inline void set_compression(const std::string& c) { _compression = c; set_defined(); }

    void read_config(const YAML::Node& node);
  };

  //! JP2 parameters for destination
  class D_JP2 : public Role_Definable {
  private:
    definable<int> _numresolutions;
    std::string _prog_order;
    std::vector<float> _rates, _qualities;
    definable< std::pair<int, int> > _tile_size;
    definable<bool> _reversible;

  public:
    //! Empty constructor
    D_JP2();

    //! Set values from a map of "variables"
    void add_variables(multihash& vars);

    inline definable<int> numresolutions(void) const { return _numresolutions; }
    inline void set_numresolutions(int n) { _numresolutions = n; set_defined(); };

    inline std::string prog_order(void) const { return _prog_order; }
    inline void set_prog_order(const std::string& po) { _prog_order = po; set_defined(); }

    inline int num_rates(void) const { return _rates.size(); }
    inline float rate(int n) const { return _rates[n]; }
    inline void set_rate(int n, float r) { _rates[n] = r; set_defined(); }
    inline void set_rates(std::vector<float> r) { _rates = r; set_defined(); }

    inline int num_qualities(void) const { return _qualities.size(); }
    inline float quality(int n) const { return _qualities[n]; }
    inline void set_quality(int n, float r) { _qualities[n] = r; set_defined(); }
    inline void set_qualities(std::vector<float> r) { _qualities = r; set_defined(); }

    inline definable< std::pair<int, int> > tile_size(void) const { return _tile_size; }
    inline void set_tile_size(int h, int v) { _tile_size = std::pair<int, int>(h, v); set_defined(); }

    inline definable<bool> reversible(void) const { return _reversible; }
    inline void set_reversible(bool r = true) { _reversible = r; }
    inline void set_irreversible(void) { _reversible = false; }

    void read_config(const YAML::Node& node);
  };

  //! WebP parameters for destination
  class D_WebP : public Role_Definable {
  private:
    std::string _preset;
    definable<bool> _lossless;
    float _quality;			// 0 -> 100
    definable<unsigned char> _method;	// 0 -> 6

  public:
    //! Empty constructor
    D_WebP();

    //! Set values from a map of "variables"
    void add_variables(multihash& vars);

    inline std::string preset(void) const { return _preset; }
    inline void set_preset(const std::string& p) { _preset = p; }

    inline definable<bool> lossless(void) const { return _lossless; }
    inline definable<bool> lossy(void) const { return !_lossless; }
    inline void set_lossless(bool l = true) { _lossless = l; }
    inline void set_lossy(bool l = true) { _lossless = !l; }

    inline float quality(void) const { return _quality; }
    inline void set_quality(float q) { _quality = q; }

    inline definable<unsigned char> method(void) const { return _method; }
    inline void set_method(unsigned char m) { _method = m; }

    void read_config(const YAML::Node& node);
  };

  //! JPEG XR parameters for destination
  class D_JXR : public Role_Definable {
  private:
    int _imageq, _alphaq;
    std::string _overlap;
    std::string _subsampling;
    definable<int> _tilesize;
    definable<bool> _progressive;

  public:
    //! Empty constructor
    D_JXR();

    //! Set values from a map of "variables"
    void add_variables(multihash& vars);

    inline int quality(void) const { return _imageq; }
    inline void set_quality(int iq) { _imageq = iq; }

    inline int alphaq(void) const { return _alphaq; }
    inline void set_alphaq(int aq) { _alphaq = aq; }

    inline std::string overlap(void) const { return _overlap; }
    inline void set_overlap(const std::string &o) { _overlap = o; }

    inline std::string subsampling(void) const { return _subsampling; }
    inline void set_subsampling(const std::string &s) { _subsampling = s; }

    inline definable<int> tilesize(void) const { return _tilesize; }
    inline void set_tilesize(int ts) { _tilesize = ts; }

    inline definable<bool> progressive(void) const { return _progressive; }
    inline void set_progressive(bool p = true) { _progressive = p; }
    inline void set_sequential(bool s = true) { _progressive = !s; }

    void read_config(const YAML::Node& node);
  };


  //! FLIF parameters for destinations
  class D_FLIF : public Role_Definable {
  private:
    bool _interlaced, _crc, _alpha_zero, _channel_compact, _ycocg;
    uint32_t _learn_repeat, _divisor, _min_size, _split_threshold;
    uint32_t _chance_cutoff, _chance_alpha, _loss;

  public:
    //! Empty constructor
    D_FLIF();

    //! Set values from a map of "variables"
    void add_variables(multihash& vars);

    inline bool interlaced(void) const { return _interlaced; }
    inline bool crc(void) const { return _crc; }
    inline bool alpha_zero(void) const { return _alpha_zero; }
    inline bool channel_compact(void) const { return _channel_compact; }
    inline bool ycocg(void) const { return _ycocg; }
    inline bool learn_repeat(void) const { return _learn_repeat; }
    inline bool divisor(void) const { return _divisor; }
    inline bool min_size(void) const { return _min_size; }
    inline bool split_threshold(void) const { return _split_threshold; }
    inline bool chance_cutoff(void) const { return _chance_cutoff; }
    inline bool chance_alpha(void) const { return _chance_alpha; }
    inline bool loss(void) const { return _loss; }

    void read_config(const YAML::Node& node);
  }; // class D_FLIF


  //! HEIF parameters
  class D_HEIF : public Role_Definable {
  private:
    int _lossy_quality;
    bool _lossless;

  public:
    //! Empty constructor
    D_HEIF();

    //! Set values from a map of "variables"
    void add_variables(multihash& vars);

    inline int lossy_quality(void) const { return _lossy_quality; }
    inline void set_lossy_quality(int lq) { _lossy_quality = lq; }

    inline bool lossless(void) const { return _lossless; }
    inline void set_lossless(bool l = true) { _lossless = l; }
    inline void set_lossy(bool l = true) { _lossless = !l; }

    void read_config(const YAML::Node& node);
  }; // class D_HEIF


  class D_JXL : public Role_Definable {
  private:
    bool _lossless;
    definable<int> _effort;
    definable<float> _distance;

  public:
    //! Empty constructor
    D_JXL();

    //! Set values from a map of "variables"
    void add_variables(multihash& vars);

    inline bool lossless(void) const { return _lossless; }
    inline void set_lossless(bool l = true) { _lossless = l; }

    inline definable<int> effort(void) const { return _effort; }
    inline void set_effort(int e) { _effort = e; }

    inline definable<float> distance(void) const { return _distance; }
    inline void set_distance(float d) { _distance = d; }

    void read_config(const YAML::Node& node);
  }; // class D_JXL


  //! ICC profile parameters for destination
  class D_profile {
  private:
    std::string _name;
    fs::path _filepath;
    unsigned char *_data;
    unsigned int _data_size;

  public:
    //! Empty constructor
    D_profile();

    //! Constructor
    D_profile(const std::string& name, fs::path filepath);

    //! Constructor
    D_profile(const std::string& name, unsigned char* data, unsigned int data_size);

    //! Copy constructor
    D_profile(const D_profile& other);

    //! Destructor
    ~D_profile();

    //! Assignment operator
    D_profile& operator=(const D_profile& b);

    //! Name of the profile
    inline std::string name(void) const { return _name; }

    //! File path for reading the profile
    inline fs::path filepath(void) const { return _filepath; }

    //! Do we have the profile data instead of a file path?
    inline bool has_data(void) const { return _data != nullptr; }

    //! The profile data for LCMS2
    CMS::Profile::ptr profile(void) const;

    //! The profile data
    inline unsigned char* data(void) const { return _data; }

    //! The size of the profile data
    inline unsigned int data_size(void) const { return _data_size; }

    void read_config(const YAML::Node& node);

    //! Shared pointer for a D_profile
    typedef std::shared_ptr<D_profile> ptr;
  };

  //! Thumbnail parameters for destination
  class D_thumbnail : public Role_Definable {
  private:
    definable<bool> _generate;
    definable<double> _maxwidth, _maxheight;

  public:
    D_thumbnail();

    inline definable<bool> generate(void) const { return _generate; }
    inline definable<double> maxwidth(void) const { return _maxwidth; }
    inline definable<double> maxheight(void) const { return _maxheight; }

    void read_config(const YAML::Node& node);
  };

}
