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
#include <lcms2.h>
#include "Exception.hh"

namespace PhotoFinish {

  //! Throw a LibraryError exception whem LCMS2 returns an error
  void lcms2_errorhandler(cmsContext ContextID, cmsUInt32Number ErrorCode, const char *Text) {
    throw LibraryError("LCMS2", Text);
  }

  void lcms2_error_adaptor(void) {
    cmsSetLogErrorHandler(lcms2_errorhandler);
  }

}
