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



  D_TIFF::D_TIFF()
  {}

  void D_TIFF::add_variables(multihash& vars) {
    multihash::iterator vi;
    if (!_artist.defined() && ((vi = vars.find("artist")) != vars.end())) {
      _artist = vi->second[0];
      vars.erase(vi);
      set_defined();
    }
    if (!_copyright.defined() && ((vi = vars.find("copyright")) != vars.end())) {
      _copyright = vi->second[0];
      vars.erase(vi);
      set_defined();
    }
    if (!_compression.defined() && ((vi = vars.find("compression")) != vars.end())) {
      _compression = vi->second[0];
      vars.erase(vi);
      set_defined();
    }
  }

  void operator >> (const YAML::Node& node, D_TIFF& dt) {
    if (const YAML::Node *n = node.FindValue("artist"))
      *n >> dt._artist;

    if (const YAML::Node *n = node.FindValue("copyright"))
      *n >> dt._copyright;

    if (const YAML::Node *n = node.FindValue("compression"))
      *n >> dt._compression;

    dt.set_defined();
  }




  D_JP2::D_JP2()
  {}

  void D_JP2::add_variables(multihash& vars) {
    multihash::iterator vi;
    if (!_numresolutions.defined() && ((vi = vars.find("numresolutions")) != vars.end())) {
      _numresolutions = boost::lexical_cast<int>(vi->second[0]);
      vars.erase(vi);
      set_defined();
    }

    if (!_prog_order.defined() && ((vi = vars.find("prog_order")) != vars.end())) {
      _prog_order = vi->second[0];
      vars.erase(vi);
      set_defined();
    }

    if ((_rates.size() == 0) && ((vi = vars.find("rate")) != vars.end())) {
      std::string rate = vi->second[0];
      size_t sep = rate.find_first_of(",/");
      while (sep != std::string::npos) {
	try {
	  _rates.push_back(boost::lexical_cast<float>(rate.substr(0, sep)));
	  vars.erase(vi);
	  set_defined();
	  rate = rate.substr(sep + 1, rate.length() - sep -1);
	} catch (boost::bad_lexical_cast &ex) {
	  std::cerr << ex.what();
	  break;
	}
	sep = rate.find_first_of(",/");
      }
      try {
	_rates.push_back(boost::lexical_cast<float>(rate));
      } catch (boost::bad_lexical_cast &ex) {
      }
    }

    if (!_tile_size.defined() && ((vi = vars.find("tile")) != vars.end())) {
      std::string tile_size = vi->second[0];
      size_t sep = tile_size.find_first_of("x×/,");
      if (sep != std::string::npos) {
	try {
	  int h = boost::lexical_cast<int>(tile_size.substr(0, sep));
	  int v = boost::lexical_cast<int>(tile_size.substr(sep + 1, tile_size.length() - sep - 1));
	  _tile_size = std::pair<int, int>(h, v);
	  vars.erase(vi);
	  set_defined();
	} catch (boost::bad_lexical_cast &ex) {
	  std::cerr << ex.what();
	}
      } else
	std::cerr << "D_JP2: Failed to parse tile size \"" << tile_size << "\"." << std::endl;
    }
  }

  void operator >> (const YAML::Node& node, D_JP2& dj) {
    if (const YAML::Node *n = node.FindValue("numresolutions"))
      *n >> dj._numresolutions;

    if (const YAML::Node *n = node.FindValue("prog_order"))
      *n >> dj._prog_order;

    if (const YAML::Node *n = node.FindValue("rate")) {
      std::string rate;
      *n >> rate;
      size_t sep = rate.find_first_of(",/");
      while (sep != std::string::npos) {
	try {
	  dj._rates.push_back(boost::lexical_cast<float>(rate.substr(0, sep)));
	  rate = rate.substr(sep + 1, rate.length() - sep -1);
	} catch (boost::bad_lexical_cast &ex) {
	  std::cerr << ex.what();
	  break;
	}
	sep = rate.find_first_of(",/");
      }
      try {
	dj._rates.push_back(boost::lexical_cast<float>(rate));
      } catch (boost::bad_lexical_cast &ex) {
      }
    }

    if (const YAML::Node *n = node.FindValue("tile")) {
      std::string tile_size;
      *n >> tile_size;
      size_t sep = tile_size.find_first_of("x×/,");
      if (sep != std::string::npos) {
	try {
	  int h = boost::lexical_cast<int>(tile_size.substr(0, sep));
	  int v = boost::lexical_cast<int>(tile_size.substr(sep + 1, tile_size.length() - sep - 1));
	  dj._tile_size = std::pair<int, int>(h, v);
	} catch (boost::bad_lexical_cast &ex) {
	  std::cerr << ex.what();
	}
      } else
	std::cerr << "D_JP2: Failed to parse tile size \"" << tile_size << "\"." << std::endl;
    }
    dj.set_defined();
  }




  D_profile::D_profile() :
    _data(NULL), _data_size(0)
  {}

  D_profile::D_profile(std::string name, fs::path filepath) :
    _name(name),
    _filepath(filepath),
    _data(NULL), _data_size(0)
  {}

  D_profile::D_profile(std::string name, void *data, unsigned int data_size) :
    _name(name),
    _data(data), _data_size(data_size)
  {}

  D_profile::D_profile(const D_profile& other) :
    _name(other._name),
    _filepath(other._filepath),
    _data(memcpy(malloc(other._data_size), other._data, other._data_size)),
    _data_size(other._data_size)
  {}

  D_profile::~D_profile() {
    if (_data != NULL) {
      free(_data);
      _data = NULL;
      _data_size = 0;
    }
  }

  D_profile& D_profile::operator=(const D_profile& b) {
    if (this != &b) {
      _name = b._name;
      _filepath = b._filepath;
      _data = malloc(b._data_size);
      memcpy(_data, b._data, b._data_size);
      _data_size = b._data_size;
    }

    return *this;
  }

  cmsHPROFILE D_profile::profile(void) const {
    if (_filepath.defined())
      return cmsOpenProfileFromFile(_filepath->c_str(), "r");
    if (_data != NULL)
      return cmsOpenProfileFromMem(_data, _data_size);

    throw Uninitialised("D_profile", "filepath and data");
  }

  //! Read a D_profile record from a YAML file
  void operator >> (const YAML::Node& node, D_profile& dp) {
    if (const YAML::Node *n = node.FindValue("name"))
      *n >> dp._name;

    if (const YAML::Node *n = node.FindValue("filename"))
      *n >> dp._filepath;
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

}
