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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Ditherer.hh"

namespace PhotoFinish {

  Ditherer::Ditherer(unsigned int width, unsigned char channels) :
    _width(width), _channels(channels),
    _error_rows(NULL),
    _curr_row(0), _next_row(1)
  {
    _error_rows = (short int**)malloc(2 * sizeof(short int*));
    for (unsigned int y = 0; y < 2; y++)
      _error_rows[y] = (short int*)malloc(_width * _channels * sizeof(short int));
    memset(_error_rows[_next_row], 0, _width * _channels * sizeof(short int));
  }

  Ditherer::~Ditherer() {
    if (_error_rows != NULL) {
      for (int y = 0; y < 2; y++)
	free(_error_rows[y]);
      free(_error_rows);
      _error_rows = NULL;
    }
  }

#define pos ((x * _channels) + c)
#define prevpos (((x - 1) * _channels) + c)
#define nextpos (((x + 1) * _channels) + c)

  void Ditherer::dither(short unsigned int *inrow, unsigned char *outrow, bool lastrow) {
    _curr_row = _next_row;
    _next_row = 1 - _curr_row;
    memset(_error_rows[_next_row], 0, _width * _channels * sizeof(short int));
#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned char c = 0; c < _channels; c++) {
      short unsigned int *in = &inrow[c];
      unsigned char *out = &outrow[c];

      unsigned int x = 0;
      if (lastrow) {
	// All but last pixel
	for (; x < _width - 1; x++, in += _channels, out += _channels) {
	  int target = *in + (_error_rows[_curr_row][pos] >> 4);
	  *out = round(target * (1.0 / 257));
	  int error = target - ((int)*out * 257);

	  _error_rows[_curr_row][nextpos] += error * 7;
	}
	// Last pixel
	if (x < _width) {
	  int target = *in + (_error_rows[_curr_row][pos] >> 4);
	  *out = round(target * (1.0 / 257));
	}
      } else {
	// First pixel
	int target = *in + (_error_rows[_curr_row][pos] >> 4);
	*out = round(target * (1.0 / 257));
	int error = target - ((int)*out * 257);

	_error_rows[_next_row][pos] += error * 5;
	if (x < _width - 1) {
	  _error_rows[_next_row][nextpos] += error;
	  _error_rows[_curr_row][nextpos] += error * 7;
	}

	x++;
	in += _channels;
	out += _channels;
	// Most pixels
	while (x < _width - 1) {
	  target = *in + (_error_rows[_curr_row][pos] >> 4);
	  *out = round(target * (1.0 / 257));
	  error = target - ((int)*out * 257);
	  _error_rows[_next_row][prevpos] += error * 3;
	  _error_rows[_next_row][nextpos] += error;
	  _error_rows[_curr_row][nextpos] += error * 7;
	  x++;
	  in += _channels;
	  out += _channels;
	}
	// Last pixel
	if (x < _width) {
	  target = *in + (_error_rows[_curr_row][pos] >> 4);
	  *out = round(target * (1.0 / 257));
	  error = target - ((int)*out * 257);
	  _error_rows[_next_row][prevpos] += error * 3;
	}
      }
    }
  }

}
