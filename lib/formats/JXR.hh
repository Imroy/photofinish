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
#ifndef __JXR_HH__
#define __JXR_HH__

#include <JXRGlue.h>
#undef min
#undef max

#include <vector>
#include <lcms2.h>
#include "CMS.hh"

#define JXRcheck(exp) if ((rc = (exp)) < 0) throw LibraryError("jxrlib", std::string(#exp) + " returned " + std::to_string(rc))

namespace PhotoFinish {

  typedef std::vector<std::pair<unsigned int, const PKPixelFormatGUID*> > jxr_format_subst;

#define FmtPair(n, g) std::make_pair<unsigned int, const PKPixelFormatGUID*>(n, g)

  const PKPixelFormatGUID& jxr_pixel_format(unsigned int n);

  CMS::Format jxr_cms_format(const PKPixelFormatGUID &g);

}

#endif // __JXR_HH__
