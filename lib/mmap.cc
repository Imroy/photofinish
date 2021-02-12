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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <setjmp.h>
#include <signal.h>
#include "mmap.hh"
#include "Exception.hh"

thread_local volatile bool sigbus_jmp_set = false;
thread_local sigjmp_buf sigbus_jmp_buf;

static void handle_sigbus(int c) {
  if (sigbus_jmp_set) {
    sigbus_jmp_set = false;
    siglongjmp(sigbus_jmp_buf, c);
  }
}

void install_signal_handlers(void) {
  struct sigaction act;
  act.sa_handler = &handle_sigbus;

  act.sa_flags = SA_NODEFER;
  sigemptyset(&act.sa_mask);

  sigaction(SIGBUS, &act, nullptr);
}

MemMap::MemMap(fs::path filepath) {
  struct stat64 st;

  if (stat64(filepath.c_str(), &st))
    throw PhotoFinish::FileOpenError(filepath.native(), "Could not stat64");

  // Open the file in read only mode
  int fd = open(filepath.c_str(), O_RDONLY);
  if (fd < 0)
    throw PhotoFinish::FileOpenError(filepath.native());

  // Allocate a buffer for the file contents
  _data = ::mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

  // mmap returns MAP_FAILED on error, not NULL
  if (_data == MAP_FAILED)
    throw PhotoFinish::MemAllocError("Could not mmap " + filepath.native());

  _length = st.st_size;
}

//! Deconstructor
MemMap::~MemMap() {
  munmap(_data, _length);
}

size_t MemMap::length(void) const {
  return _length;
}

