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
#include "Image.hh"
#include <stdio.h>

int main(int argc, char* argv[]) {
  Image image1("test3.png");

  /*
  unsigned int w, h;
  w = image1.width();
  h = image1.height();
  for (unsigned int y = 0; y < h; y++)
    for (unsigned int x = 0; x < w; x++) {
      double *p = image1.at(x, y);
      fprintf(stderr, "at(%d, %d) = (%f, %f, %f)\n", x, y, p[0], p[1], p[2]);
    }
  */

  //  image1.write_png("test3.out.png", 8, NULL, INTENT_PERCEPTUAL);

  Image image2 = image1.resize(800, -1, 3);
  image2.write_png("test3.scaled.png", 8, NULL, INTENT_PERCEPTUAL);

  return 0;
}
