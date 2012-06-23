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
#include "ImageFile.hh"
#include <stdio.h>

int main(int argc, char* argv[]) {
  PNGFile infile("test3.png");
  Image *image1 = infile.read();

  Image *image2 = image1->resize(800, -1, 3);
  delete image1;

  PNGFile outfile("test3.scaled.png");
  outfile.set_bit_depth(8);
  outfile.write(image2);
  delete image2;

  return 0;
}
