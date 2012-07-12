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
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <deque>
#include <boost/filesystem.hpp>
#include "Image.hh"
#include "ImageFile.hh"
#include "Exception.hh"
#include "Destination.hh"
#include "Tags.hh"

namespace fs = boost::filesystem;

using namespace PhotoFinish;

int main(int argc, char* argv[]) {
  if (argc == 1) {
    std::cerr << argv[0] << "<input file> [<input file>...] <destination> [<destination>...]" << std::endl;
    exit(1);
  }

  Destinations destinations("destinations.yml");

  std::deque<std::string> arg_destinations;
  std::deque<fs::path> arg_filenames;
  for (int i = 1; i < argc; i++) {
    struct stat s;
    if (destinations.count(argv[i]))
      arg_destinations.push_back(argv[i]);
    else if (stat(argv[i], &s) == 0)
      arg_filenames.push_back(argv[i]);
    else
      std::cerr << "Argument \"" << argv[i] << " is neither a destination name, nor a filename." << std::endl;
  }

  for (std::deque<fs::path>::iterator fi = arg_filenames.begin(); fi != arg_filenames.end(); fi++) {
    try {
      ImageFile::ptr infile = ImageFile::create(*fi);
      Tags tags;
      if (fs::exists(".tags/default"))
	tags.load(".tags/default");
      {
	fs::path tagpath = (*fi).parent_path() / ("." + (*fi).stem().native() + ".tags");
	if (fs::exists(tagpath))
	  tags.load(tagpath);
      }

      try {
	Image::ptr image = infile->read();

	for (std::deque<std::string>::iterator di = arg_destinations.begin(); di != arg_destinations.end(); di++) {
	  Destination::ptr destination = destinations[*di]->add_variables(tags.variables());
	  try {
	    Image::ptr outimage;
	    if (destination->has_noresize() && destination->noresize()) {
	      outimage = image;
	    } else {
	      Frame::ptr frame = destination->best_frame(image);
	      Filter::ptr filter = Filter::create(destination->resize());
	      outimage = frame->crop_resize(image, filter);
	      if (destination->has_forcergb() && destination->forcergb())
		outimage->set_colour();
	    }

	    if (destination->has_size())
	      tags.add_resolution(outimage, destination);

	    if (destination->has_thumbnail() && destination->thumbnail().has_generate() && destination->thumbnail().generate())
	      tags.make_thumbnail(outimage, destination->thumbnail());

	    if (!exists(destination->dir())) {
	      std::cerr << "Creating directory " << destination->dir() << "." << std::endl;
	      create_directory(destination->dir());
	    }
	    ImageFile::ptr outfile = ImageFile::create(destination->dir() / (*fi).stem(), destination->has_format() ? destination->format() : "jpeg");
	    outfile->write(outimage, *destination, tags);
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

  return 0;
}
