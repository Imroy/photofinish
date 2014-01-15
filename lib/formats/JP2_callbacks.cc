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
#include <string.h>
#include "Exception.hh"

namespace PhotoFinish {

  void error_callback(const char* msg, void* client_data) {
    throw LibraryError("OpenJPEG", msg);
  }

  void warning_callback(const char* msg, void* client_data) {
    ((char*)msg)[strlen(msg) - 1] = 0;
    std::cerr << "** OpenJPEG:" << msg << " **" << std::endl;
  }

  void info_callback(const char* msg, void* client_data) {
    ((char*)msg)[strlen(msg) - 1] = 0;
    std::cerr << "\tOpenJPEG:" << msg << std::endl;
  }

} // namespace PhotoFinish
