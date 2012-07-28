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
#ifndef __CROPSOLUTION_HH__
#define __CROPSOLUTION_HH__

#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include "Frame.hh"

namespace PhotoFinish {

  typedef std::pair<double, double> targetpair;
  typedef std::vector< targetpair > targetlist;

  class CropSolver {
  private:
    targetlist _h_targets, _v_targets;

  public:
    CropSolver(multihash& vars);

    Frame::ptr solve(Image::ptr img, D_target::ptr target);
  };

};

#endif /* __CROPSOLUTION_H__ */
