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
#include <boost/lexical_cast.hpp>
#include <math.h>
#include "CropSolution.hh"

#define sqr(x) ((x) * (x))
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

namespace PhotoFinish {

  CropSolver::CropSolver(Image::ptr img, multihash& vars) {
    multihash::iterator vi;
    stringlist::iterator si;
    bool has_left = false, has_right = false;
    if ((vi = vars.find("targetx")) != vars.end()) {
      for (si = vi->second.begin(); si != vi->second.end(); si++) {
	int at = si->find_first_of('@', 0);
	double fx = boost::lexical_cast<double>(si->substr(0, at));
	if (fx < 0.5)
	  has_left = true;
	else if (fx > 0.5)
	  has_right = true;
	double tx = boost::lexical_cast<double>(si->substr(at + 1, si->length() - at - 1));
	_h_targets.push_back(std::make_pair(fx, tx));
      }
    }
    if (_h_targets.size() < 2) {
      if (!has_left)
	_h_targets.push_back(std::pair<double, double>(0, 0));
      if (!has_right)
	_h_targets.push_back(std::pair<double, double>(1, img->width() - 1));
    }

    bool has_top = false, has_bottom = false;
    if ((vi = vars.find("targety")) != vars.end()) {
      for (si = vi->second.begin(); si != vi->second.end(); si++) {
	int at = si->find_first_of('@', 0);
	double fy = boost::lexical_cast<double>(si->substr(0, at));
	if (fy < 0.5)
	  has_top = true;
	else if (fy > 0.5)
	  has_bottom = true;
	double ty = boost::lexical_cast<double>(si->substr(at + 1, si->length() - at - 1));
	_v_targets.push_back(std::make_pair(fy, ty));
      }
    }
    if (_v_targets.size() < 2) {
      if (!has_top)
	_v_targets.push_back(std::pair<double, double>(0, 0));
      if (!has_bottom)
	_v_targets.push_back(std::pair<double, double>(1, img->height() - 1));
    }

  }

  Frame::ptr CropSolver::solve(Image::ptr img, D_target::ptr target) {
    Frame::ptr best_frame;
    double best_weight = 0;

    double imgsize = max(img->width(), img->height());
    double tsize = max(target->width(), target->height());
    double width_factor = target->width() / tsize;
    double height_factor = target->height() / tsize;

    double step = 16;
    double minx = 0, maxx = img->width();
    double miny = 0, maxy = img->height();
    double minsize = step, maxsize = step;
    bool first = true;

    while (step > 1e-6) {
      // OpenMP (or at least GOMP) can't parallelise a for loop using floating point values
      // So put them into a list and loop over that
      std::vector<double> xvalues;
      for (double x = minx; x < maxx; x += step)
	xvalues.push_back(x);

#pragma omp parallel for schedule(dynamic, 1) shared(xvalues, best_frame, best_weight)
      for (size_t xi = 0; xi < xvalues.size(); xi++) {
	double x = xvalues[xi];
	for (double y = miny; y < maxy; y += step) {
	  if (first)
	    maxsize = min((img->width() - x) / width_factor, (img->height() - y) / height_factor);
	  for (double size = minsize; size < maxsize; size += step) {
	    double width = size * width_factor;
	    if (x + width > img->width())
	      continue;
	    double height = size * height_factor;
	    if (y + height > img->height())
	      continue;

	    double weight = 0;

	    std::vector< std::pair<double, double> >::iterator ti;

	    for (ti = _h_targets.begin(); ti != _h_targets.end(); ti++) {
	      double fx = x + (ti->first * width);
	      weight += sqr(ti->second - fx);
	    }

	    for (ti = _v_targets.begin(); ti != _v_targets.end(); ti++) {
	      double fy = y + (ti->first * height);
	      weight += sqr(ti->second - fy);
	    }

#pragma omp critical
	    if ((!best_frame) || (weight < best_weight)) {
	      Frame::ptr new_best_frame(new Frame(*target, x, y, width, height, 0));
	      best_frame.swap(new_best_frame);
	      best_weight = weight;
	    }
	  }
	}
      }

      minx = best_frame->crop_x() - step;
      maxx = best_frame->crop_x() + step;
      if (minx < 0)
	minx = 0;
      else if (maxx > img->width())
	maxx = img->width();

      miny = best_frame->crop_y() - step;
      maxy = best_frame->crop_y() + step;
      if (miny < 0)
	miny = 0;
      else if (maxy > img->height())
	maxy = img->height();

      double best_size = max(best_frame->crop_w(), best_frame->crop_h());
      minsize = best_size - step;
      maxsize = best_size + step;
      if (minsize < 0)
	minsize = 0;
      else if (maxsize > imgsize)
	maxsize = imgsize;

      step /= 16;
      first = false;
    }

    return best_frame;
  }


};
