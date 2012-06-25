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
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <deque>
#include "yaml-cpp/yaml.h"
#include "Image.hh"
#include "ImageFile.hh"
#include "Destination.hh"

int main(int argc, char* argv[]) {
  std::ifstream fin("destinations.yml");
  YAML::Parser parser(fin);
  YAML::Node doc;
  parser.GetNextDocument(doc);

  map<std::string, Destination*> destinations;
  for(YAML::Iterator it = doc.begin(); it != doc.end(); it++) {
    std::string destname;
    it.first() >> destname;
    fprintf(stderr, "Destination \"%s\".\n", destname.c_str());
    Destination *destination = new Destination;
    it.second() >> *destination;
    destinations[destname] = destination;
  }

  PNGFile infile("test3.png");
  Image *image1 = infile.read();

  _ImageFile *outfile = ImageFile("test3.scaled.jpeg");
  if (outfile != NULL) {
    Image *image2 = image1->resize(1200, -1, 3);
    delete image1;

    outfile->write(image2, *destinations["wall2"]);
    delete image2;
    delete outfile;
  } else
    delete image1;

  for (map<string, Destination*>::iterator di = destinations.begin(); di != destinations.end(); di++)
    delete di->second;

  return 0;
}
