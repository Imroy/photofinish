/*
	Copyright 2021 Ian Tester

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
#pragma once

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

// Using code from https://www.sublimetext.com/blog/articles/use-mmap-with-care

class MemMap {
private:
  void *_data = nullptr;
  size_t _length = 0;

public:
  //! Constructor
  MemMap(fs::path filepath);

  //! Deconstructor
  ~MemMap();

  template <typename T=uint8_t>
  T* data(void)  {
    return (T*)_data;
  }

  size_t length(void) const;

}; // class mmap

void install_signal_handlers(void);

extern thread_local volatile bool sigbus_jmp_set;
extern thread_local sigjmp_buf sigbus_jmp_buf;

template <typename F>
bool safe_mmap_try(F fn) {
  assert(!sigbus_jmp_set);

  sigbus_jmp_set = true;

  if (sigsetjmp(sigbus_jmp_buf, 0) == 0) {
    fn();

    sigbus_jmp_set = false;
    return true;
  }

  sigbus_jmp_set = false;
  return false;
}
