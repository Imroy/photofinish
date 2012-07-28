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
#include <omp.h>
#include "CropSolution.hh"

#define sqr(x) ((x) * (x))
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

namespace PhotoFinish {

  void add_rulers(multihash& vars, std::string key, rulerlist& rulers) {
    multihash::iterator vi;
    if ((vi = vars.find(key)) != vars.end())
      for (stringlist::iterator si = vi->second.begin(); si != vi->second.end(); si++) {
	int at = si->find_first_of('@', 0);
	double fx = boost::lexical_cast<double>(si->substr(0, at));
	double tx = boost::lexical_cast<double>(si->substr(at + 1, si->length() - at - 1));
	rulers.push_back(std::make_pair(fx, tx));
      }
  }

  CropSolver::CropSolver(multihash& vars) {
    add_rulers(vars, "hruler", _h_rulers);
    add_rulers(vars, "vruler", _v_rulers);
  }

  void add_ruler_pins(rulerlist& rulers, unsigned int max) {
    bool has_first = false, has_last = false;
    for (rulerlist::iterator ti = rulers.begin(); ti != rulers.end(); ti++) {
      if (ti->first < 0.5)
	has_first = true;
      else if (ti->first > 0.5)
	has_last = true;
    }

    if (!has_first)
      rulers.push_back(rulerpair(0, 0));
    if (!has_last)
      rulers.push_back(rulerpair(1, max - 1));
  }

  Frame::ptr CropSolver::solve(Image::ptr img, D_target::ptr target) {
    rulerlist h_rulers(_h_rulers), v_rulers(_v_rulers);
    if ((target->width() * img->height() > target->height() * img->width())
	&& (h_rulers.size() < 2))
      add_ruler_pins(h_rulers, img->width());

    if ((target->width() * img->height() < target->height() * img->width())
	&& (v_rulers.size() < 2))
      add_ruler_pins(v_rulers, img->height());

    Frame::ptr best_frame;
    double best_distance = 0;
    omp_lock_t best_lock;
    omp_init_lock(&best_lock);

    double imgsize = max(img->width(), img->height());
    double tsize = max(target->width(), target->height());
    double width_factor = target->width() / tsize;
    double height_factor = target->height() / tsize;

    double step = 8;
    double minx = 0, maxx = img->width();
    double miny = 0, maxy = img->height();
    double minsize = step, maxsize = step;
    bool first = true, new_best = false;

    while (step > 1e-7) {
      // OpenMP can't parallelise a for loop using floating point values
      // So put them into a list and loop over that
      std::vector<double> xvalues;
      for (double x = minx; x < maxx; x += step)
	xvalues.push_back(x);

#pragma omp parallel for schedule(dynamic, 1)
      for (size_t xi = 0; xi < xvalues.size(); xi++) {
	double x = xvalues[xi];
	double xsize = (img->width() - x) / width_factor;

	for (double y = miny; y < maxy; y += step) {
	  if (first)
	    maxsize = min(xsize, (img->height() - y) / height_factor);

	  for (double size = minsize; size < maxsize; size += step) {
	    double width = size * width_factor;
	    if (x + width > img->width())
	      continue;
	    double height = size * height_factor;
	    if (y + height > img->height())
	      continue;

	    double distance = 0;

	    rulerlist::iterator ti;

	    for (ti = h_rulers.begin(); (ti != h_rulers.end()) && ((!best_frame) || (distance < best_distance)); ti++)
	      distance += sqr(ti->second - (x + (ti->first * width)));

	    if ((best_frame) && (distance > best_distance))
	      continue;

	    for (ti = v_rulers.begin(); (ti != v_rulers.end()) && ((!best_frame) || (distance < best_distance)); ti++)
	      distance += sqr(ti->second - (y + (ti->first * height)));

	    if (best_frame && (distance > best_distance))
	      continue;

	    omp_set_lock(&best_lock);
	    if ((!best_frame) || (distance < best_distance)) {
	      Frame::ptr new_best_frame(new Frame(*target, x, y, width, height, 0));
	      best_frame.swap(new_best_frame);
	      best_distance = distance;
	      new_best = true;
	    }
	    omp_unset_lock(&best_lock);
	  }
	}
      }

      if (!new_best)
	break;

      minx = best_frame->crop_x() - step * 2;
      maxx = best_frame->crop_x() + step * 2;
      if (minx < 0)
	minx = 0;
      else if (maxx > img->width())
	maxx = img->width();

      miny = best_frame->crop_y() - step * 2;
      maxy = best_frame->crop_y() + step * 2;
      if (miny < 0)
	miny = 0;
      else if (maxy > img->height())
	maxy = img->height();

      double best_size = max(best_frame->crop_w(), best_frame->crop_h());
      minsize = best_size - step * 2;
      maxsize = best_size + step * 2;
      if (minsize < 0)
	minsize = 0;
      else if (maxsize > imgsize)
	maxsize = imgsize;

      step /= 16;
      first = false;
    }

    if (best_frame)
      std::cerr << "\t\tBest frame (" << best_frame->crop_x() << ", " << best_frame->crop_y() << ") + ("
		<< best_frame->crop_w() << "×" << best_frame->crop_h() << ") (distance = " << sqrt(best_distance) << ")" << std::endl;

    return best_frame;
  }


};
