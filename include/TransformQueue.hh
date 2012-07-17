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
#ifndef __TRANSFORMQUEUE_HH__
#define __TRANSFORMQUEUE_HH__

#include <queue>
#include <list>
#include <vector>
#include <omp.h>
#include <lcms2.h>
#include "Image.hh"

namespace PhotoFinish {

  //! Class holding information for the image writers
  class transform_queue {
  private:
    std::queue<unsigned int, std::list<unsigned int> > _rownumbers;
    short unsigned int **_rows;
    std::vector<omp_lock_t*> _rowlocks;
    unsigned char **_destrows;
    size_t _rowlen;
    omp_lock_t *_queue_lock;
    Image::ptr _img;
    cmsHTRANSFORM _transform;

  public:
    //! Constructor
    /*!
      \param img The Image object from which the pixel data comes
      \param channels The number of channels in the output
      \param transform The LCMS2 transform for transforming the image data
      \param destrows For 16-bit output, the row pointers into which to place the transformed pixels; if NULL we allocate our own rows
    */
    transform_queue(Image::ptr img, int channels, cmsHTRANSFORM transform, unsigned char **destrows = NULL);

    //! Deconstructor
    ~transform_queue();

    //! The number of rows in the queue
    inline size_t num_rows(void) const { return _rownumbers.size(); }

    //! Is the queue empty?
    bool empty(void) const;

    //! Return a row pointer
    short unsigned int* row(unsigned int y) const;

    //! Pull a row off of the workqueue and transform it using LCMS
    void process_row(void);
  };

}

#endif /* __TRANSFORMQUEUE_HH__ */
