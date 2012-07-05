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

  Ditherer::Ditherer(long int w, unsigned char c) :
    width(w), channels(c),
    error_rows(NULL),
    curr_row(0), next_row(1)
  {
    error_rows = (short int**)malloc(2 * sizeof(short int*));
    for (long int y = 0; y < 2; y++)
      error_rows[y] = (short int*)malloc(width * channels * sizeof(short int));
    memset(error_rows[next_row], 0, width * channels * sizeof(short int));
  }

  Ditherer::~Ditherer() {
    if (error_rows != NULL) {
      for (int y = 0; y < 2; y++)
	free(error_rows[y]);
      free(error_rows);
      error_rows = NULL;
    }
  }

#define pos ((x * channels) + c)
#define prevpos (((x - 1) * channels) + c)
#define nextpos (((x + 1) * channels) + c)

  void Ditherer::dither(short unsigned int *inrow, unsigned char *outrow, bool lastrow) {
    curr_row = next_row;
    next_row = 1 - curr_row;
    memset(error_rows[next_row], 0, width * channels * sizeof(short int));
#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned char c = 0; c < channels; c++) {
      short unsigned int *in = &inrow[c];
      unsigned char *out = &outrow[c];

      if (lastrow) {
	for (long int x = 0; x < width - 1; x++, in += channels, out += channels) {
	  int target = *in + (error_rows[curr_row][pos] >> 4);
	  *out = round(target * (1.0 / 257));
	  int error = target - ((int)*out * 257);

	  error_rows[curr_row][nextpos] += error * 7;
	}
      } else {
	// First pixel
	long int x = 0;
	int target = *in + (error_rows[curr_row][pos] >> 4);
	*out = round(target * (1.0 / 257));
	int error = target - ((int)*out * 257);

	error_rows[next_row][pos] += error * 5;
	if (x < width - 1) {
	  error_rows[next_row][nextpos] += error;
	  error_rows[curr_row][nextpos] += error * 7;
	}

	x++;
	in += channels;
	out += channels;
	// Most pixels
	while (x < width - 1) {
	  target = *in + (error_rows[curr_row][pos] >> 4);
	  *out = round(target * (1.0 / 257));
	  error = target - ((int)*out * 257);
	  error_rows[next_row][prevpos] += error * 3;
	  error_rows[next_row][nextpos] += error;
	  error_rows[curr_row][nextpos] += error * 7;
	  x++;
	  in += channels;
	  out += channels;
	}
	// Last pixel
	if (x < width) {
	  target = *in + (error_rows[curr_row][pos] >> 4);
	  *out = round(target * (1.0 / 257));
	  error = target - ((int)*out * 257);
	  error_rows[next_row][prevpos] += error * 3;
	}
      }
    }
  }

}
