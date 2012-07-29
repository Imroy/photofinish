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
#include "Destination.hh"
#include "Tags.hh"
#include "Kernel2D.hh"
#include "Exception.hh"

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
	    definable<double> size = destination->size();

	    Image::ptr sized_image;
	    if (destination->noresize().defined() && destination->noresize()) {
	      sized_image = image;
	    } else {
	      Frame::ptr frame = destination->best_frame(image);
	      sized_image = frame->crop_resize(image, destination->resize());
	      if (destination->forcergb().defined() && destination->forcergb())
		sized_image->set_colour();
	      if (destination->forcegrey().defined() && destination->forcegrey())
		sized_image->set_greyscale();
	      if (frame->size().defined())
		size = frame->size();
	    }

	    Image::ptr sharp_image;
	    if (destination->sharpen().defined()) {
	      Kernel2D::ptr sharpen = Kernel2D::create(destination->sharpen());
	      sharp_image = sharpen->convolve(sized_image);
	    } else {
	      sharp_image = sized_image;
	    }
	    sized_image.reset();	// Unallocate resized image

	    if (size.defined()) {
	      sharp_image->set_resolution_from_size(size);
	      tags.add_resolution(sharp_image);
	    }

	    if (destination->thumbnail().defined() && destination->thumbnail().generate().defined() && destination->thumbnail().generate())
	      tags.make_thumbnail(sharp_image, destination->thumbnail());

	    if (!exists(destination->dir())) {
	      std::cerr << "Creating directory " << destination->dir() << "." << std::endl;
	      create_directory(destination->dir());
	    }
	    std::string format = "jpeg";
	    if (destination->format().defined())
	      format = destination->format();
	    ImageFile::ptr outfile = ImageFile::create(destination->dir() / (*fi).stem(), format);
	    outfile->write(sharp_image, *destination, tags);
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
