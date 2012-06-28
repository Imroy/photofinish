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
#ifndef __DESTINATION_HH__
#define __DESTINATION_HH__

#include "yaml-cpp/yaml.h"
#include <string>
#include <map>
#include "Destination_items.hh"
#include "Image.hh"
#include "Frame.hh"

namespace PhotoFinish {

  class Destination {
  private:
    bool _has_name, _has_dir;
    std::string _name, _dir;		// Name of this destination and the destination directory

    bool _has_size;
    double _size;			// Size in inches (yuck)

    bool _has_sharpen, _has_resize;
    D_sharpen _sharpen;
    D_resize _resize;
    std::map<std::string, D_target*> _targets;

    bool _has_format;
    std::string _format;

    bool _has_depth;
    int _depth;

    bool _has_noresize, _noresize;

    bool _has_jpeg, _has_png;
    D_JPEG _jpeg;
    D_PNG _png;

    bool _has_intent;
    cmsUInt32Number _intent;

    bool _has_profile;
    D_profile _profile;

  public:
    Destination();
    Destination(const Destination& other);
    ~Destination();

    const Frame& best_frame(const Image& img);

    inline bool has_name(void) const { return _has_name; }
    inline std::string name(void) const { return _name; }

    inline bool has_dir(void) const { return _has_dir; }
    inline std::string dir(void) const { return _dir; }

    inline bool has_size(void) const { return _has_size; }
    inline double size(void) const { return _size; }

    inline bool has_sharpen(void) const { return _has_sharpen; }
    inline const D_sharpen& sharpen(void) const { return _sharpen; }

    inline bool has_resize(void) const { return _has_resize; }
    inline const D_resize& resize(void) const { return _resize; }

    inline int num_targets(void) const { return _targets.size(); }
    inline bool has_targets(void) const { return !_targets.empty(); }
    inline const std::map<std::string, D_target*>& targets(void) const { return _targets; }

    inline bool has_format(void) const { return _has_format; }
    inline std::string format(void) const { return _format; }

    inline bool has_depth(void) const { return _has_depth; }
    inline int depth(void) const { return _depth; }

    inline bool has_noresize(void) const { return _has_noresize; }
    inline bool noresize(void) const { return _noresize; }

    inline bool has_png(void) const { return _has_png; }
    inline const D_PNG& png(void) const { return _png; }

    inline bool has_jpeg(void) const { return _has_jpeg; }
    inline const D_JPEG& jpeg(void) const { return _jpeg; }

    inline bool has_intent(void) const { return _has_intent; }
    inline cmsUInt32Number intent(void) const { return _intent; }

    inline bool has_profile(void) const { return _has_profile; }
    inline const D_profile& profile(void) const { return _profile; }

    friend void operator >> (const YAML::Node& node, Destination& d);
  };

}

#endif // __DESTINATION_HH__
