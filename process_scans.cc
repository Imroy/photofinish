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
#include "Image.hh"
#include "ImageFile.hh"
#include "Destination.hh"
#include "Tags.hh"
#include "Kernel2D.hh"
#include "Exception.hh"

namespace fs = boost::filesystem;
namespace fs3 = boost::filesystem3;
namespace po = boost::program_options;

using namespace PhotoFinish;

int main(int argc, char* argv[]) {
  bool do_conversion, do_preview;
  po::options_description generic_options("Generic options");
  generic_options.add_options()
    ("help,h", "produce help message")
    ("convert,C", po::bool_switch(&do_conversion), "Convert files")
    ("preview,P", po::bool_switch(&do_preview), "Scale the image to a preview")
    ;

  std::string convert_dir, convert_format, preview_format;
  typedef std::vector<std::string> stringlist;
  stringlist include_path;
  po::options_description config_options("Configuration");
  config_options.add_options()
    ("convert-dir", po::value<std::string>(&convert_dir)->default_value("originals")->composing(), "Directory to place converted files")
    ("convert-format", po::value<std::string>(&convert_format)->default_value("png")->composing(), "Format of converted images")
    ("preview-format", po::value<std::string>(&preview_format)->default_value("jpeg")->composing(), "Format of preview images")
    ("include-path,I", po::value< stringlist >(&include_path)->composing(), "include path for tag files")
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

  po::variables_map opts;
  po::store(po::parse_command_line(argc, argv, cmdline_options), opts);
  if (fs::exists(configdir / "process_scans.ini")) {
    fs::ifstream config_file(configdir / "process_scans.ini", std::ios_base::in);
    po::store(po::parse_config_file(config_file, config_options), opts);
  }
  po::notify(opts);

  if (opts.count("help")) {
    std::cerr << visible_options << std::endl;
    return 1;
  }

  Tags::ptr defaulttags(new Tags);
  for (stringlist::iterator i = include_path.begin(); i != include_path.end(); i++)
    defaulttags->add_searchpath(*i);

  defaulttags->try_load("default");
  if (fs::exists(".tags"))
    defaulttags->load(".tags");

  std::vector<fs::directory_entry> dir_list;
  for (fs::directory_iterator di(fs::current_path()); di != fs::directory_iterator(); di++)
    dir_list.push_back(*di);
  sort(dir_list.begin(), dir_list.end());
  for (std::vector<fs::directory_entry>::iterator di = dir_list.begin(); di != dir_list.end(); di++) {
    if (!boost::iequals(di->path().extension().generic_string(), ".tif")
	&& !boost::iequals(di->path().extension().generic_string(), ".tiff"))
      continue;

    try {
      ImageFile::ptr infile = ImageFile::create(di->path());

      Destination::ptr orig_dest(new Destination);
      Image::ptr orig_image = infile->read(orig_dest);
      Tags::ptr filetags = defaulttags->dupe();
      filetags->extract(infile);

      if (do_conversion) {
	if (!fs::exists(convert_dir)) {
	  std::cerr << "Creating directory \"" << convert_dir << "\"." << std::endl;
	  fs::create_directory(convert_dir);
	}

	ImageFile::ptr converted_file = ImageFile::create(convert_dir / di->path().stem(), convert_format);
	converted_file->write(orig_image, orig_dest);
	filetags->embed(converted_file);
      }

      if (do_preview) {
	Destination::ptr resized_dest = orig_dest->dupe();

	resized_dest->set_depth(8);
	resized_dest->set_jpeg(D_JPEG(60, 1, 1, true));

	Frame::ptr frame(new Frame(orig_image->width() * 0.25, orig_image->height() * 0.25,
				   0, 0,
				   orig_image->width(), orig_image->height()));

	Image::ptr resized_image = frame->crop_resize(orig_image, D_resize::lanczos(3));
	ImageFile::ptr preview_file = ImageFile::create(di->path().stem(), preview_format);
	preview_file->write(resized_image, resized_dest);
	filetags->embed(preview_file);
      }

      std::cerr << "Moving " << di->path() << " to \"" << convert_dir << "/" << di->path().filename().native() << "\"" << std::endl;
      rename(di->path().c_str(), (convert_dir / di->path().filename()).c_str());
    } catch (std::exception& ex) {
      std::cerr << ex.what() << std::endl;
    }
  }
}
