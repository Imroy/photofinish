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
#include "Rescaler.hh"
#include "Sharpen.hh"
#include "Exception.hh"

namespace fs = boost::filesystem;
namespace fs3 = boost::filesystem3;
namespace po = boost::program_options;

using namespace PhotoFinish;

void make_preview(ImageSource::ptr source, WorkGang::ptr workgang, Destination::ptr orig_dest, Tags::ptr filetags, ImageFilepath preview_file) {
  Destination::ptr resized_dest = orig_dest->dupe();

  resized_dest->set_depth(8);
  resized_dest->set_jpeg(D_JPEG(60, 1, 1, true));
  resized_dest->clear_profile();

  ImageWriter::ptr preview_writer = ImageWriter::open(preview_file, resized_dest);
  preview_writer->add_end_handler([=] { filetags->embed(preview_file); });
  workgang->add_worker(preview_writer);
  add_FixedFactorRescaler(source, preview_writer, workgang, 0.25);
}

void preview_dir(fs::path dir, std::string preview_format, Tags::ptr tags, std::vector<std::string> only_formats = {}, std::vector<std::string> not_formats = {}) {
  std::vector<fs::directory_entry> dir_list;
  for (fs::directory_iterator di(dir); di != fs::directory_iterator(); di++)
    dir_list.push_back(*di);
  sort(dir_list.begin(), dir_list.end());

  for (std::vector<fs::directory_entry>::iterator di = dir_list.begin(); di != dir_list.end(); di++) {
    std::cerr << "preview_dir: " << *di << std::endl;
    try {
      WorkGang::ptr workgang(new WorkGang);
      ImageFilepath infile(di->path());
      if (only_formats.size() > 0) {
	bool valid = false;
	std::string format = infile.format();
	std::cerr << "format = \"" << format << "\"" << std::endl;
	for (std::vector<std::string>::iterator fi = only_formats.begin(); fi != only_formats.end(); fi++) {
	  std::cerr << "only_format = \"" << *fi << "\"" << std::endl;
	  if (boost::iequals(format, *fi)) {
	    std::cerr << "Format \"" << format << "\" matches \"" << *fi << "\", valid." << std::endl;
	    valid = true;
	    break;
	  }
	}
	if (!valid)
	  continue;
      } else if (not_formats.size() > 0) {
	bool valid = true;
	std::string format = infile.format();
	std::cerr << "format = \"" << format << "\"" << std::endl;
	for (std::vector<std::string>::iterator fi = not_formats.begin(); fi != not_formats.end(); fi++) {
	  std::cerr << "not_format = \"" << *fi << "\"" << std::endl;
	  if (boost::iequals(format, *fi)) {
	    std::cerr << "Format \"" << format << "\" matches \"" << *fi << "\", invalid." << std::endl;
	    valid = false;
	    break;
	  }
	}
	if (!valid)
	  continue;
      }

      ImageFilepath preview_file(di->path().filename(), preview_format);

      if (exists(preview_file.filepath())
	  && (last_write_time(preview_file.filepath()) > last_write_time(infile.filepath())))
	continue;

      Destination::ptr orig_dest(new Destination);
      ImageReader::ptr reader = ImageReader::open(infile);
      workgang->add_worker(reader);
      Tags::ptr filetags = tags->dupe();
      filetags->extract(infile);

      make_preview(reader, workgang, orig_dest, filetags, preview_file);
      workgang->work_until_finished();
    } catch (std::exception& ex) {
      std::cerr << ex.what() << std::endl;
    }
  }
}

int main(int argc, char* argv[]) {
  // Variables that are to be loaded from the config file and command line
  bool do_conversion, do_preview;
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

  Tags::ptr defaulttags(new Tags);
  // Add paths for tag file searching
  for (pathlist::iterator i = include_paths.begin(); i != include_paths.end(); i++)
    defaulttags->add_searchpath(*i);

  // Load "default" in the search path list
  defaulttags->try_load("default");
  // Load ".tags" from the current directory
  if (fs::exists(".tags"))
    defaulttags->load(".tags");

  if (do_conversion || do_preview) {
    // std::sort won't work on fs::directory_iterator, so we have to put the entries in a vector and sort that
    std::vector<fs::directory_entry> dir_list;
    for (fs::directory_iterator di(fs::current_path()); di != fs::directory_iterator(); di++)
      dir_list.push_back(*di);
    sort(dir_list.begin(), dir_list.end());

    // Iterate over all of the TIFF files in the current directory
    for (std::vector<fs::directory_entry>::iterator di = dir_list.begin(); di != dir_list.end(); di++) {
      if (!boost::iequals(di->path().extension().generic_string(), ".tif")
	  && !boost::iequals(di->path().extension().generic_string(), ".tiff"))
	continue;

      try {
	WorkGang::ptr workgang(new WorkGang);
	ImageFilepath infile(di->path());
	ImageReader::ptr reader = ImageReader::open(infile);
	workgang->add_worker(reader);

	Destination::ptr orig_dest(new Destination);
	Tags::ptr filetags = defaulttags->dupe();
	filetags->extract(infile);

	try {
	  ImageFilepath converted_file(convert_dir / di->path().filename(), convert_format);
	  if (do_conversion && (!exists(converted_file.filepath()) || (last_write_time(converted_file.filepath()) < last_write_time(infile.filepath())))) {
	    if (!fs::exists(convert_dir)) {
	      std::cerr << "Creating directory " << convert_dir << "." << std::endl;
	      fs::create_directory(convert_dir);
	    }
	    reader->add_header_handler([=] (ImageHeader::ptr header) {
					 ImageWriter::ptr converted_writer = ImageWriter::open(converted_file, orig_dest);
					 converted_writer->add_end_handler([=] { filetags->embed(converted_file); });
					 workgang->add_worker(converted_writer);
					 reader->add_sink(converted_writer);
				       });
	  }
	} catch (std::exception& ex) {
	  std::cerr << ex.what() << std::endl;
	}

	try {
	  ImageFilepath preview_file(di->path().filename(), preview_format);
	  if (do_preview && (!exists(preview_file.filepath()) || (last_write_time(preview_file.filepath()) < last_write_time(infile.filepath()))))
	    make_preview(reader, workgang, orig_dest, filetags, preview_file);
	} catch (std::exception& ex) {
	  std::cerr << ex.what() << std::endl;
	}

	workgang->work_until_finished();

	if (do_conversion) {
	  std::cerr << "Moving " << di->path() << " to " << convert_dir / di->path().filename().native() << std::endl;
	  rename(di->path().c_str(), (convert_dir / di->path().filename()).c_str());
	}
      } catch (std::exception& ex) {
	std::cerr << ex.what() << std::endl;
      }
    }
  }

  if (do_preview) {
    preview_dir(convert_dir, preview_format, defaulttags, { "tiff", "png" });
    if (!exists(works_dir)) {
      std::cerr << "Creating directory " << works_dir << "." << std::endl;
      create_directory(works_dir);
    } else
      preview_dir(works_dir, preview_format, defaulttags);
  }
}
