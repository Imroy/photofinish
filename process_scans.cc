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
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "CMS.hh"
#include "Image.hh"
#include "ImageFile.hh"
#include "Destination.hh"
#include "Tags.hh"
#include "Kernel2D.hh"
#include "Exception.hh"

namespace fs = boost::filesystem;
namespace po = boost::program_options;

using namespace PhotoFinish;

void make_preview(Image::ptr orig_image, Destination::ptr orig_dest, Tags::ptr filetags, ImageFile::ptr preview_file, bool can_free = false) {
  CMS::Format orig_format = orig_image->format();
  orig_format.set_colour_model(CMS::ColourModel::Lab);
  orig_format.set_channels(3);
  SET_SAMPLE_FORMAT(orig_format);
  orig_image->transform_colour_inplace(CMS::Profile::Lab4(), orig_format);

  auto resized_dest = orig_dest->dupe();

  resized_dest->set_depth(8);
  resized_dest->clear_profile();
  resized_dest->set_jpeg(D_JPEG(60, 2, 2, true));
  resized_dest->jp2().set_rates({ 120, 12 });
  resized_dest->jp2().set_tile_size(1024, 1024);
  resized_dest->jp2().set_numresolutions(5);
  resized_dest->tiff().set_compression("deflate");
  resized_dest->webp().set_preset("photo");
  resized_dest->webp().set_quality(80);
  resized_dest->webp().set_method(6);

  auto frame = std::make_shared<Frame>(orig_image->width() * 0.25, orig_image->height() * 0.25,
				       0, 0,
				       orig_image->width(), orig_image->height());

  auto resized_image = frame->crop_resize(orig_image, D_resize::lanczos(3), true);

  CMS::Format dest_format = preview_file->preferred_format(resized_dest->modify_format(resized_image->format()));
  CMS::Profile::ptr dest_profile = resized_dest->get_profile(dest_format.colour_model());
  resized_image->transform_colour_inplace(dest_profile, dest_format);

  filetags->copy_to(resized_image);
  preview_file->write(resized_image, resized_dest, can_free);
}

void preview_dir(fs::path dir, std::string format, std::shared_ptr<Tags> tags) {
  std::vector<fs::directory_entry> dir_list;
  for (auto di = fs::directory_iterator(dir); di != fs::directory_iterator(); di++)
    dir_list.push_back(*di);
  sort(dir_list.begin(), dir_list.end());

  for (auto di : dir_list) {
    std::cerr << di << std::endl;
    try {
      auto infile = ImageFile::create(di.path());
      auto preview_file = ImageFile::create(di.path().filename(), format);

      if (exists(preview_file->filepath())
        && (last_write_time(preview_file->filepath()) > last_write_time(infile->filepath())))
        continue;

      auto orig_dest = std::make_shared<Destination>();
      auto orig_image = infile->read(orig_dest);
      auto filetags = tags->dupe();
      filetags->copy_from(orig_image);

      make_preview(orig_image, orig_dest, filetags, preview_file, true);
    } catch (std::exception& ex) {
      std::cerr << ex.what() << std::endl;
    }
  }
}

int main(int argc, char* argv[]) {
  // Variables that are to be loaded from the config file and command line
  bool do_conversion, do_preview, do_move_originals;
  fs::path convert_dir, works_dir;
  std::string convert_format, preview_format;
  typedef std::vector<fs::path> pathlist;
  pathlist include_paths;

  // Define and load config file and command line options
  po::variables_map opts;
  {
    po::options_description generic_options("Generic options");
    generic_options.add_options()
      ("help,h", "produce help message")
      ("convert,C", po::bool_switch(&do_conversion), "Convert files")
      ("preview,P", po::bool_switch(&do_preview), "Scale the image to a preview")
      ("move-originals,M", po::bool_switch(&do_move_originals), "Move originals (done automatically when converting)")
      ;

    po::options_description config_options("Configuration");
    config_options.add_options()
      ("convert-dir", po::value<fs::path>(&convert_dir)->default_value("originals"), "Directory to place converted files")
      ("convert-format", po::value<std::string>(&convert_format)->default_value("png"), "Format of converted images")
      ("preview-format", po::value<std::string>(&preview_format)->default_value("jpeg"), "Format of preview images")
      ("works-dir", po::value<fs::path>(&works_dir)->default_value("works"), "Directory to find works in progress")
      ("include-path,I", po::value< pathlist >(&include_paths)->composing(), "include path for tag files")
      ;

    po::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_options);

    po::options_description visible_options("Allowed options");
    visible_options.add(generic_options).add(config_options);

    fs::path homedir = getenv("HOME");
    fs::path configdir = homedir / ".config/PhotoFinish";
    if (!fs::exists(configdir)) {
      std::cerr << "Creating " << configdir << std::endl;
      fs::create_directory(configdir);
    }

    po::store(po::parse_command_line(argc, argv, cmdline_options), opts);
    if (fs::exists(configdir / "process_scans.ini")) {
      fs::ifstream config_file(configdir / "process_scans.ini", std::ios_base::in);
      po::store(po::parse_config_file(config_file, config_options), opts);
    }
    po::notify(opts);

    // Display descriptions of command-line options if '--help' is given
    if (opts.count("help")) {
      std::cerr << visible_options << std::endl;
      exit(1);
    }
  }

  lcms2_error_adaptor();

  auto defaulttags = std::make_shared<Tags>();
  // Add paths for tag file searching
  for (auto i : include_paths)
    defaulttags->add_searchpath(i);

  // Load "default" in the search path list
  defaulttags->try_load("default");
  // Load ".tags" from the current directory
  if (fs::exists(".tags"))
    defaulttags->load(".tags");

  if (do_conversion || do_preview) {
    // std::sort won't work on fs::directory_iterator, so we have to put the entries in a vector and sort that
    std::vector<fs::directory_entry> dir_list;
    for (auto di = fs::directory_iterator(fs::current_path()); di != fs::directory_iterator(); di++)
      dir_list.push_back(*di);
    sort(dir_list.begin(), dir_list.end());

    // Iterate over all of the TIFF files in the current directory
    for (auto di : dir_list) {
      if (!boost::iequals(di.path().extension().generic_string(), ".tif")
	  && !boost::iequals(di.path().extension().generic_string(), ".tiff"))
	continue;

      try {
	auto infile = ImageFile::create(di.path());

	auto orig_dest = std::make_shared<Destination>();
	auto orig_image = infile->read(orig_dest);
	auto filetags = defaulttags->dupe();
	filetags->copy_from(orig_image);

	try {
	  auto converted_file = ImageFile::create(convert_dir / di.path().filename(), convert_format);
	  if (do_conversion && (!exists(converted_file->filepath()) || (last_write_time(converted_file->filepath()) < last_write_time(infile->filepath())))) {
	    if (!fs::exists(convert_dir)) {
	      std::cerr << "Creating directory " << convert_dir << "." << std::endl;
	      fs::create_directory(convert_dir);
	    }

	    auto converted_dest = orig_dest->dupe();

	    // Set various format options to give best quality i.e lossless compression or the closest to it
	    converted_dest->set_jpeg(D_JPEG(100, 1, 1, true));
	    converted_dest->jp2().set_rates({ 1000, 100, 10, 1 });
	    converted_dest->jp2().set_tile_size(2048, 2048);
	    converted_dest->jp2().set_numresolutions(7);
	    converted_dest->tiff().set_compression("deflate");
	    converted_dest->webp().set_preset("photo");
	    converted_dest->webp().set_lossless();
	    converted_dest->webp().set_method(6);

	    CMS::Format orig_format = orig_image->format();
	    CMS::Format converted_format = converted_file->preferred_format(converted_dest->modify_format(orig_format));
	    Image::ptr converted_image;
	    if (orig_format == converted_format)
	      converted_image = orig_image;
	    else
	      converted_image = orig_image->transform_colour(NULL, converted_format);

	    filetags->copy_to(converted_image);
	    converted_file->write(converted_image, converted_dest, !do_preview);
	  }
	} catch (std::exception& ex) {
	  std::cerr << ex.what() << std::endl;
	}

	try {
	  auto preview_file = ImageFile::create(di.path().filename(), preview_format);
	  if (do_preview && (!exists(preview_file->filepath()) || (last_write_time(preview_file->filepath()) < last_write_time(infile->filepath()))))
	    make_preview(orig_image, orig_dest, filetags, preview_file, true);
	} catch (std::exception& ex) {
	  std::cerr << ex.what() << std::endl;
	}

	if (do_conversion || do_move_originals) {
	  if (!fs::exists(convert_dir)) {
	    std::cerr << "Creating directory " << convert_dir << "." << std::endl;
	    fs::create_directory(convert_dir);
	  }
	  std::cerr << "Moving " << di.path() << " to " << convert_dir / di.path().filename().native() << std::endl;
	  rename(di.path().c_str(), (convert_dir / di.path().filename()).c_str());
	}
      } catch (std::exception& ex) {
	std::cerr << ex.what() << std::endl;
      }
    }
  }

  if (do_preview) {
    if (exists(convert_dir))
      preview_dir(convert_dir, preview_format, defaulttags);

    if (exists(works_dir))
      preview_dir(works_dir, preview_format, defaulttags);
    else {
      std::cerr << "Creating directory " << works_dir << "." << std::endl;
      create_directory(works_dir);
    }
  }
}
