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
#ifndef __DITHERER_HH__
#define __DITHERER_HH__

namespace PhotoFinish {

  class Ditherer {
  private:
    long int width;
    unsigned char channels;
    short int **error_rows;
    int curr_row, next_row;

  public:
    Ditherer(long int w, unsigned char c);
    ~Ditherer();

    void dither(short unsigned int *inrow, unsigned char *outrow);
  };

}

#endif /* __DITHERER_HH__ */
