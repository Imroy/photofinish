/*
	Copyright 2014 Ian Tester

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
#include <boost/algorithm/string/predicate.hpp>
#include <string.h>
#include "Destination_items.hh"
#include "Destination.hh"
#include "CropSolution.hh"
#include "Exception.hh"

namespace PhotoFinish {

  D_sharpen::D_sharpen()
  {}

  //! Read a D_sharpen record from a YAML file
  void D_sharpen::read_config(const YAML::Node& node) {
    if (node["radius"])
      _radius = node["radius"].as<double>();

    if (node["sigma"])
      _sigma = node["sigma"].as<double>();

    set_defined();
  }



  D_resize::D_resize()
  {}

  D_resize::D_resize(const std::string& f, double s) :
    _filter(f), _support(s)
  {}

  //! Read a D_resize record from a YAML file
  void D_resize::read_config(const YAML::Node& node) {
    if (node["filter"])
      _filter = node["filter"].as<std::string>();

    if (node["support"])
      _support = node["support"].as<double>();

    set_defined();
  }



  D_target::D_target(const std::string& n, double w, double h) :
    _name(n),
    _width(w), _height(h)
  {}

  D_target::D_target(const std::string& n) :
    _name(n)
  {}

  //! Read a D_target record from a YAML file
  void D_target::read_config(const YAML::Node& node) {
    if (node["width"])
      _width = node["width"].as<double>();

    if (node["height"])
      _height = node["height"].as<double>();

    if (node["size"])
      _size = node["size"].as<double>();
  }



  D_JPEG::D_JPEG()
  {}

  D_JPEG::D_JPEG(int q, char h, char v, bool p) :
    _quality(q),
    _sample(std::pair<int, int>(h, v)),
    _progressive(p)
  {}

  void D_JPEG::add_variables(multihash& vars) {
    if (!_quality.defined()) {
      auto vi = vars.find("qual");
      if (vi != vars.end()) {
	_quality = boost::lexical_cast<int>(vi->second[0]);
	vars.erase(vi);
	set_defined();
      }
    }
    if (!_sample.defined()) {
      auto vi = vars.find("sample");
      if (vi != vars.end()) {
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
  }

  //! Read a D_JPEG record from a YAML file
  void D_JPEG::read_config(const YAML::Node& node) {
    if (node["qual"])
      _quality = node["qual"].as<int>();

    if (node["sample"]) {
      std::string sample = node["sample"].as<std::string>();
      size_t x = sample.find_first_of("x×");
      if (x != std::string::npos) {
	try {
	  int h = boost::lexical_cast<int>(sample.substr(0, x));
	  int v = boost::lexical_cast<int>(sample.substr(x + 1, sample.length() - x - 1));
	  _sample = std::pair<int, int>(h, v);
	} catch (boost::bad_lexical_cast &ex) {
	  std::cerr << ex.what();
	}
      } else
	std::cerr << "D_JPEG: Failed to parse sample \"" << sample << "\"." << std::endl;
    }

    if (node["pro"])
      _progressive = node["pro"].as<bool>();

    set_defined();
  }



  D_PNG::D_PNG() {
  }

  //! Read a D_PNG record from a YAML file
  void D_PNG::read_config(const YAML::Node& node) {
  }



  D_TIFF::D_TIFF()
  {}

  D_TIFF::D_TIFF(const std::string& c)
    : _compression(c)
  {}

  void D_TIFF::add_variables(multihash& vars) {
    if (!_artist.defined()) {
      auto vi = vars.find("artist");
      if (vi != vars.end()) {
	_artist = vi->second[0];
	vars.erase(vi);
	set_defined();
      }
    }
    if (!_copyright.defined()) {
      auto vi = vars.find("copyright");
      if (vi != vars.end()) {
	_copyright = vi->second[0];
	vars.erase(vi);
	set_defined();
      }
    }
    if (!_compression.defined()) {
      auto vi = vars.find("compression");
      if (vi != vars.end()) {
	_compression = vi->second[0];
	vars.erase(vi);
	set_defined();
      }
    }
  }

  //! Read a D_TIFF record from a YAML file
  void D_TIFF::read_config(const YAML::Node& node) {
    if (node["artist"])
      _artist = node["artist"].as<std::string>();

    if (node["copyright"])
      _copyright = node["copyright"].as<std::string>();

    if (node["compression"])
      _compression = node["compression"].as<std::string>();

    set_defined();
  }




  D_JP2::D_JP2() :
    _rates(),
    _qualities()
  {}

  void D_JP2::add_variables(multihash& vars) {
    if (!_numresolutions.defined()) {
      auto vi = vars.find("numresolutions");
      if (vi != vars.end()) {
	_numresolutions = boost::lexical_cast<int>(vi->second[0]);
	vars.erase(vi);
	set_defined();
      }
    }

    if (!_prog_order.defined()) {
      auto vi = vars.find("prog_order");
      if (vi != vars.end()) {
	_prog_order = vi->second[0];
	vars.erase(vi);
	set_defined();
      }
    }

    if (_rates.size() == 0) {
      auto vi = vars.find("rate");
      if (vi != vars.end()) {
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
    }

    if (_qualities.size() == 0) {
      auto vi = vars.find("quality");
      if (vi != vars.end()) {
	std::string quality = vi->second[0];
	size_t sep = quality.find_first_of(",/");
	while (sep != std::string::npos) {
	  try {
	    _qualities.push_back(boost::lexical_cast<float>(quality.substr(0, sep)));
	    vars.erase(vi);
	    set_defined();
	    quality = quality.substr(sep + 1, quality.length() - sep -1);
	  } catch (boost::bad_lexical_cast &ex) {
	    std::cerr << ex.what();
	    break;
	  }
	  sep = quality.find_first_of(",/");
	}
	try {
	  _qualities.push_back(boost::lexical_cast<float>(quality));
	} catch (boost::bad_lexical_cast &ex) {
	}
      }
    }

    if (!_tile_size.defined()) {
      auto vi = vars.find("tile");
      if (vi != vars.end()) {
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

    if (!_reversible.defined()) {
      if (vars.find("reversible") != vars.end())
	_reversible = true;
      else if (vars.find("irreversible") != vars.end())
	_reversible = false;
    }
  }

  //! Read a D_JP2 record from a YAML file
  void D_JP2::read_config(const YAML::Node& node) {
    if (node["numresolutions"])
      _numresolutions = node["numresolutions"].as<int>();

    if (node["prog_order"])
      _prog_order = node["prog_order"].as<std::string>();

    if (node["rate"]) {
      std::string rate = node["rate"].as<std::string>();
      size_t sep = rate.find_first_of(",/");
      while (sep != std::string::npos) {
	try {
	  _rates.push_back(boost::lexical_cast<float>(rate.substr(0, sep)));
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

    if (node["quality"]) {
      std::string quality = node["quality"].as<std::string>();
      size_t sep = quality.find_first_of(",/");
      while (sep != std::string::npos) {
	try {
	  _qualities.push_back(boost::lexical_cast<float>(quality.substr(0, sep)));
	  quality = quality.substr(sep + 1, quality.length() - sep -1);
	} catch (boost::bad_lexical_cast &ex) {
	  std::cerr << ex.what();
	  break;
	}
	sep = quality.find_first_of(",/");
      }
      try {
	_qualities.push_back(boost::lexical_cast<float>(quality));
      } catch (boost::bad_lexical_cast &ex) {
      }
    }

    if (node["tile"]) {
      std::string tile_size = node["tile"].as<std::string>();
      size_t sep = tile_size.find_first_of("x×/,");
      if (sep != std::string::npos) {
	try {
	  int h = boost::lexical_cast<int>(tile_size.substr(0, sep));
	  int v = boost::lexical_cast<int>(tile_size.substr(sep + 1, tile_size.length() - sep - 1));
	  _tile_size = std::pair<int, int>(h, v);
	} catch (boost::bad_lexical_cast &ex) {
	  std::cerr << ex.what();
	}
      } else
	std::cerr << "D_JP2: Failed to parse tile size \"" << tile_size << "\"." << std::endl;
    }

    if (node["reversible"])
      _reversible = node["reversible"].as<bool>();
    else if (node["irreversible"])
      _reversible = false;

    set_defined();
  }




  D_WebP::D_WebP() :
    _quality(75)
  {}

  void D_WebP::add_variables(multihash& vars) {
  }

  void D_WebP::read_config(const YAML::Node& node) {
    if (node["preset"])
      _preset = node["preset"].as<std::string>();

    if (node["lossless"])
      _lossless = node["lossless"].as<bool>();

    if (node["quality"])
      _quality = node["quality"].as<float>();

    if (node["method"])
      _method = node["method"].as<int>();

    set_defined();
  }




  D_JXR::D_JXR() :
    _imageq(60),
    _alphaq(100),
    _subsampling("420")
  {}

  void D_JXR::add_variables(multihash& vars) {
  }

  void D_JXR::read_config(const YAML::Node& node) {
    if (node["quality"])
      _imageq = node["quality"].as<int>();

    if (node["alphaq"])
      _alphaq = node["alphaq"].as<int>();

    if (node["overlap"])
      _overlap = node["overlap"].as<std::string>();

    if (node["subsampling"])
      _subsampling = node["subsampling"].as<std::string>();

    if (node["tile"])
      _tilesize = node["tile"].as<int>();

    if (node["progressive"])
      _progressive = node["progressive"].as<bool>();

    set_defined();
  }




  D_FLIF::D_FLIF() :
    _interlaced(true),
    _crc(true),
    _alpha_zero(false),
    _channel_compact(true),
    _ycocg(true),
    _learn_repeat(2),
    _divisor(30),
    _min_size(50),
    _split_threshold(64),
    _chance_cutoff(2),
    _chance_alpha(19),
    _loss(0)
  {}

  void D_FLIF::add_variables(multihash& vars) {
  }

  void D_FLIF::read_config(const YAML::Node& node) {
    if (node["interlaced"])
      _interlaced = node["interlaced"].as<bool>();

    if (node["crc"])
      _crc = node["crc"].as<bool>();

    if (node["alpha_zero"])
      _alpha_zero = node["alpha_zero"].as<bool>();

    if (node["channel_compact"])
      _channel_compact = node["channel_compact"].as<bool>();

    if (node["ycocg"])
      _ycocg = node["ycocg"].as<bool>();

    if (node["learn_repeat"])
      _learn_repeat = node["learn_repeat"].as<uint32_t>();

    if (node["divisor"])
      _divisor = node["divisor"].as<uint32_t>();

    if (node["min_size"])
      _min_size = node["min_size"].as<uint32_t>();

    if (node["split_threshold"])
      _split_threshold = node["split_threshold"].as<uint32_t>();

    if (node["chance_cutoff"])
      _chance_cutoff = node["chance_cutoff"].as<uint32_t>();

    if (node["chance_alpha"])
      _chance_alpha = node["chance_alpha"].as<uint32_t>();

    if (node["loss"])
      _loss = node["loss"].as<uint32_t>();

    if (node["lossless"])
      _loss = 0;

  }




  D_profile::D_profile() :
    _data(nullptr), _data_size(0)
  {}

  D_profile::D_profile(const std::string& name, fs::path filepath) :
    _name(name),
    _filepath(filepath),
    _data(nullptr), _data_size(0)
  {}

  D_profile::D_profile(const std::string& name, unsigned char* data, unsigned int data_size) :
    _name(name),
    _data(data), _data_size(data_size)
  {}

  D_profile::D_profile(const D_profile& other) :
    _name(other._name),
    _filepath(other._filepath),
    _data(nullptr),
    _data_size(0)
  {
    _data = new unsigned char[other._data_size];
    memcpy(_data, other._data, other._data_size);
  }

  D_profile::~D_profile() {
    if (_data != nullptr) {
      delete [] _data;
      _data = nullptr;
      _data_size = 0;
    }
  }

  D_profile& D_profile::operator=(const D_profile& b) {
    if (this != &b) {
      _name = b._name;
      _filepath = b._filepath;
      _data = new unsigned char[b._data_size];
      memcpy(_data, b._data, b._data_size);
      _data_size = b._data_size;
    }

    return *this;
  }

  CMS::Profile::ptr D_profile::profile(void) const {
    if (_filepath.defined())
      return std::make_shared<CMS::Profile>(_filepath);
    if (_data != nullptr)
      return std::make_shared<CMS::Profile>(_data, _data_size);

    throw Uninitialised("D_profile", "filepath and data");
  }

  //! Read a D_profile record from a YAML file
  void D_profile::read_config(const YAML::Node& node) {
    if (node["name"])
      _name = node["name"].as<std::string>();

    if (node["filename"])
      _filepath = node["filename"].as<std::string>();
  }


  D_thumbnail::D_thumbnail()
  {}

  //! Read a D_thumbnail record from a YAML file
  void D_thumbnail::read_config(const YAML::Node& node) {
    if (node["generate"])
      _generate = node["generate"].as<bool>();

    if (node["maxwidth"])
      _maxwidth = node["maxwidth"].as<double>();

    if (node["maxheight"])
      _maxheight = node["maxheight"].as<double>();

    set_defined();
  }

}
