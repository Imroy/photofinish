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
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Ditherer.hh"
#include "sample.h"

namespace PhotoFinish {

  Ditherer::Ditherer(unsigned int width, unsigned char channels, std::vector<unsigned char> maxvalues) :
    _width(width), _channels(channels),
    _error_rows(NULL),
    _curr_row(0), _next_row(1),
    _maxvalues(maxvalues),
    _scale(NULL), _unscale(NULL)
  {
    _error_rows = (short int**)malloc(2 * sizeof(short int*));
    for (unsigned int y = 0; y < 2; y++)
      _error_rows[y] = (short int*)malloc(_width * _channels * sizeof(short int));
    memset(_error_rows[_next_row], 0, _width * _channels * sizeof(short int));

    _scale = (SAMPLE*)malloc(_channels * sizeof(SAMPLE));
    _unscale = (SAMPLE*)malloc(_channels * sizeof(SAMPLE));
    _maxvalues.reserve(_channels);
    for (unsigned char c = 0; c < _channels; c++) {
      if (c >= _maxvalues.size())
	_maxvalues[c] = 255;
      _scale[c] = _maxvalues[c] / 65535.0;
      _unscale[c] = 65535.0 / _maxvalues[c];
    }
  }

  Ditherer::~Ditherer() {
    if (_error_rows != NULL) {
      for (int y = 0; y < 2; y++)
	free(_error_rows[y]);
      free(_error_rows);
      _error_rows = NULL;

      free(_scale);
      _scale = NULL;
      free(_unscale);
      _unscale = NULL;
    }
  }

  inline unsigned char Ditherer::attemptvalue(int target, unsigned char channel) {
    if (target < 0)
      return 0;
    if (target > 65535)
      return _maxvalues[channel];
    return round(target * _scale[channel]);
  }

  inline int Ditherer::actualvalue(unsigned char attempt, unsigned char channel) {
    return round(attempt * _unscale[channel]);
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
	  *out = attemptvalue(target, c);
	  int error = target - actualvalue(*out, c);

	  _error_rows[_curr_row][nextpos] += error * 7;
	}
	// Last pixel
	if (x < _width) {
	  int target = *in + (_error_rows[_curr_row][pos] >> 4);
	  *out = attemptvalue(target, c);
	}
      } else {
	// First pixel
	int target = *in + (_error_rows[_curr_row][pos] >> 4);
	*out = attemptvalue(target, c);
	int error = target - actualvalue(*out, c);

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
	  *out = attemptvalue(target, c);
	  error = target - actualvalue(*out, c);
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
	  *out = attemptvalue(target, c);
	  error = target - actualvalue(*out, c);
	  _error_rows[_next_row][prevpos] += error * 3;
	}
      }
    }
  }

}
