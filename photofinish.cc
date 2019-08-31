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
#include <iostream>
#include <string>
#include <deque>
#include <boost/filesystem.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "Image.hh"
#include "ImageFile.hh"
#include "Destination.hh"
#include "Tags.hh"
#include "Kernel2D.hh"
#include "Exception.hh"
#include "Benchmark.hh"

namespace fs = boost::filesystem;

using namespace PhotoFinish;

int main(int argc, char* argv[]) {
  if (argc == 1) {
    std::cerr << argv[0] << "<input file> [<input file>...] <destination> [<destination>...]" << std::endl;
    exit(1);
  }

  lcms2_error_adaptor();

  Destinations destinations("destinations.yml");

  std::deque<std::string> arg_destinations;
  std::deque<fs::path> arg_filenames;
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "-b") {
      benchmark_mode = true;
      continue;
    }

    struct stat s;
    if (destinations.count(argv[i]))
      arg_destinations.push_back(argv[i]);
    else if (stat(argv[i], &s) == 0)
      arg_filenames.push_back(argv[i]);
    else
      std::cerr << "Argument \"" << argv[i] << " is neither a destination name, nor a filename." << std::endl;
  }

  auto defaulttags = std::make_shared<Tags>();
  defaulttags->add_searchpath(".tags");
  if (fs::exists(".tags/default"))
    defaulttags->load(".tags/default");

  for (auto fi : arg_filenames) {
    try {
      ImageFilepath infilepath(fi);
      auto infile = ImageReader::open(infilepath);
      auto tags = defaulttags->dupe();
      {
	fs::path tagpath = fi.parent_path() / ("." + fi.stem().native() + ".tags");
	if (fs::exists(tagpath))
	  tags->load(tagpath);
      }

      try {
	auto orig_image = infile->read();
	{
	  CMS::Format internal_format;
	  internal_format.set_colour_model(CMS::ColourModel::Lab);
	  SET_SAMPLE_FORMAT(internal_format);
	  internal_format.set_extra_channels(orig_image->format().extra_channels());
	  orig_image = orig_image->transform_colour(CMS::Profile::Lab4(), internal_format);
	}

	auto num_destinations = arg_destinations.size();
	for (auto& di : arg_destinations) {
	  auto destination = destinations[di]->add_variables(tags->variables());
	  try {
	    definable<double> size = destination->size();

	    Image::ptr sized_image;
	    if (destination->noresize().defined() && destination->noresize()) {
	      sized_image = orig_image;
	    } else {
	      auto frame = destination->best_frame(orig_image);
	      sized_image = frame->crop_resize(orig_image, destination->resize());
	      if (frame->size().defined())
		size = frame->size();
	    }

	    Image::ptr sharp_image;
	    if (destination->sharpen().defined()) {
	      auto sharpen = Kernel2D::create(destination->sharpen());
	      sharp_image = sharpen->convolve(sized_image);
	    } else
	      sharp_image = sized_image;
	    sized_image.reset();	// Unallocate resized image

	    if (size.defined()) {
	      sharp_image->set_resolution_from_size(size);
	      tags->add_resolution(sharp_image);
	    }

	    if (destination->thumbnail().defined() && destination->thumbnail().generate().defined() && destination->thumbnail().generate())
	      tags->make_thumbnail(sharp_image, destination->thumbnail());

	    if (!exists(destination->dir())) {
	      std::cerr << "Creating directory " << destination->dir() << "." << std::endl;
	      create_directory(destination->dir());
	    }

	    std::string format = "jpeg";
	    if (destination->format().defined())
	      format = destination->format();
	    ImageFilepath outfilepath(destination->dir() / fi.stem(), format);
	    auto outfile = ImageWriter::open(outfilepath);

	    CMS::Format dest_format = outfile->preferred_format(destination->modify_format(sharp_image->format()));
	    CMS::Profile::ptr dest_profile = destination->get_profile(dest_format.colour_model(), "destination");
	    sharp_image = sharp_image->transform_colour(dest_profile, dest_format);

	    tags->copy_to(sharp_image);
	    outfile->write(sharp_image, destination);
	  } catch (DestinationError& ex) {
	    std::cout << ex.what() << std::endl;
	    continue;
	  }

	  num_destinations--;
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
