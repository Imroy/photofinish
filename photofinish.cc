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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <deque>
#include "yaml-cpp/yaml.h"
#include "Image.hh"
#include "ImageFile.hh"
#include "Exception.hh"
#include "Destination.hh"

using namespace PhotoFinish;

int main(int argc, char* argv[]) {
  std::map<std::string, Destination*> destinations;
  {
    std::ifstream fin("destinations.yml");
    YAML::Parser parser(fin);
    YAML::Node doc;

    parser.GetNextDocument(doc);
    for (YAML::Iterator it = doc.begin(); it != doc.end(); it++) {
      std::string destname;
      it.first() >> destname;
      fprintf(stderr, "Destination \"%s\".\n", destname.c_str());
      Destination *destination = new Destination;
      it.second() >> *destination;
      destinations[destname] = destination;
    }
  }

  std::deque<std::string> arg_destinations;
  std::deque<std::string> arg_filenames;
  for (int i = 1; i < argc; i++) {
    struct stat s;
    if (destinations.count(argv[i]))
      arg_destinations.push_back(argv[i]);
    else if (stat(argv[i], &s) == 0)
      arg_filenames.push_back(argv[i]);
    else
      fprintf(stderr, "Argument \"%s\" is neither a destination name, nor a filename.\n", argv[i]);
  }

  for (std::deque<std::string>::iterator fi = arg_filenames.begin(); fi != arg_filenames.end(); fi++) {
    try {
      ImageFile infile(*fi);

      try {
	Image image = infile.read();

	for (std::deque<std::string>::iterator di = arg_destinations.begin(); di != arg_destinations.end(); di++) {
	  fprintf(stderr, "Destination: \"%s\".\n", di->c_str());

	  Destination *destination = destinations[*di];
	  Frame frame = destination->best_frame(image);
	  try {
	    Filter filter(destination->resize());
	    Image outimage = frame.crop_resize(image, filter);

	    JPEGFile outfile(*fi + "." + *di + ".jpeg");
	    outfile.write(outimage, *destination);
	  } catch (DestinationError& ex) {
	    std::cout << ex.what() << std::endl;
	    continue;
	  }
	}
      } catch (std::exception& ex) {
	std::cout << ex.what() << std::endl;
      }
    } catch (std::exception& ex) {
      std::cout << ex.what() << std::endl;
      continue;
    }

  }

  for (std::map<std::string, Destination*>::iterator di = destinations.begin(); di != destinations.end(); di++)
    delete di->second;

  return 0;
}
