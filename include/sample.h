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

// Use this to set the precision of many internal operations.
// Either 'float' (single precision) or 'double' (double precision)
#ifndef SAMPLE
#define SAMPLE float
#endif

// See CMS.hh for the set_*() methods
// TODO: Figure out the preprocessor voodoo to do essentially "set_ ## SAMPLE()"
#ifndef SET_SAMPLE_FORMAT
#define SET_SAMPLE_FORMAT(x) ((x).set_float())
#endif
