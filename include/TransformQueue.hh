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
    typedef std::pair<unsigned int, void*> row_t;

    std::queue<row_t*, std::list<row_t*> > _rowqueue;
    short unsigned int **_rowpointers;
    std::vector<omp_lock_t*> _rowlocks;
    size_t _rowlen;
    omp_lock_t *_queue_lock;
    Image::ptr _img;
    cmsHTRANSFORM _transform;
    bool _finished;

  public:
    //! Empty constructor
    transform_queue();

    //! Constructor
    /*!
      \param img The Image object from which the pixel data comes
      \param channels The number of channels in the output
      \param transform The LCMS2 transform for transforming the image data
    */
    transform_queue(Image::ptr img, int channels, cmsHTRANSFORM transform);

    //! Deconstructor
    ~transform_queue();

    //! Set the image
    void set_image(Image::ptr img, int channels);

    //! Get the image
    inline Image::ptr image(void) const { return _img; }

    //! Set the CMS transform
    inline void set_transform(cmsHTRANSFORM transform) { _transform = transform; }

    //! The number of rows in the queue
    inline size_t num_rows(void) const { return _rowqueue.size(); }

    //! Is the queue empty?
    bool empty(void) const;

    //! Is the reading process finished?
    inline bool finished(void) const { return _finished; }

    //! Signal the reading process has finished
    inline void set_finished(void) { _finished = true; }

    //! Return a row pointer
    short unsigned int* row(unsigned int y) const;

    //! Add a new row to the queue
    void add(unsigned int num, void* data = NULL);

    //! Add a new row to the queue, copying the data
    void add_copy(unsigned int num, void* data);

    //! Pull a row off of the workqueue and transform it using LCMS for a reader
    void reader_process_row(void);

    //! Pull a row off of the workqueue and transform it using LCMS for a writer
    //! If the row's data pointer is NULL, a new row is allocated for it
    void writer_process_row(void);
  };

}

#endif /* __TRANSFORMQUEUE_HH__ */
