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
    memset(error_rows[curr_row], 0, width * channels * sizeof(short int));
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

  void Ditherer::dither(short unsigned int *inrow, unsigned char *outrow) {
    memset(error_rows[next_row], 0, width * channels * sizeof(short int));
#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned char c = 0; c < channels; c++) {
      short unsigned int *in = &inrow[c];
      unsigned char *out = &outrow[c];
      for (long int x = 0; x < width; x++, in += channels, out += channels) {
	int target = *in + error_rows[curr_row][pos];
	*out = round(target * (1.0 / 257));
	int error = target - (int)(*out * 257);

	error_rows[next_row][pos] += (error * 5) >> 4;
	if (x > 0)
	  error_rows[next_row][pos - channels] += (error * 3) >> 4;
	if (x < width - 1) {
	  error_rows[next_row][pos + channels] += error >> 4;
	  error_rows[curr_row][pos + channels] += (error * 7) >> 4;
	}
      }
    }

    curr_row = next_row;
    next_row = 1 - curr_row;
  }

}
