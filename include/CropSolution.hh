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
#ifndef __CROPSOLUTION_HH__
#define __CROPSOLUTION_HH__

#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <list>
#include "Frame.hh"

namespace PhotoFinish {

  //! Ruler paramaters - percentage of final image vs. pixel position in original
  typedef std::pair<double, double> rulerpair;

  //! A list of rulers
  typedef std::list< rulerpair > rulerlist;

  //! Class for finding the best frame position for cropping
  class CropSolver {
  private:
    rulerlist _h_rulers, _v_rulers;

  public:
    CropSolver(multihash& vars);

    Frame::ptr solve(Image::ptr img, D_target::ptr target);
  };

};

#endif /* __CROPSOLUTION_H__ */
