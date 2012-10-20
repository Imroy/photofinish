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
#ifndef __IMAGE_HH__
#define __IMAGE_HH__

#include <list>
#include <memory>
#include <lcms2.h>
#include <omp.h>
#include "Definable.hh"
#include "Worker.hh"
#include "sample.h"

namespace PhotoFinish {

  class ImageRow;

  //! Image header information
  class ImageHeader {
  protected:
    unsigned int _width, _height;
    cmsHPROFILE _profile;
    cmsUInt32Number _cmsType;
    definable<double> _xres, _yres;		// PPI
    size_t _row_size;

  public:
    //! Empty constructor
    ImageHeader();

    //! Constructor
    /*!
      \param w,h Width and height of the image
    */
    ImageHeader(unsigned int w, unsigned int h);

    //! The width of this image
    inline const unsigned int width(void) const { return _width; }

    //! The height of this image
    inline const unsigned int height(void) const { return _height; }

    //! Set the ICC profile
    inline void set_profile(cmsHPROFILE p) { _profile = p; }

    //! Get the ICC profile
    inline cmsHPROFILE profile(void) const { return _profile; }

    //! Set the CMS type
    inline void set_cmsType(cmsUInt32Number t) { _cmsType = t; }

    //! Get the CMS type
    inline cmsUInt32Number cmsType(void) const { return _cmsType; }

    //! The X resolution of this image (PPI)
    inline const definable<double> xres(void) const { return _xres; }

    //! The Y resolution of this image (PPI)
    inline const definable<double> yres(void) const { return _yres; }

    //! Set both the X and Y resolution (PPI)
    inline void set_resolution(double r) { _xres = _yres = r; }

    //! Set the X resolution (PPI)
    inline void set_xres(double r) { _xres = r; }

    //! Set the Y resolution (PPI)
    inline void set_yres(double r) { _yres = r; }

    //! Set the X and Y resolutions (PPI)
    inline void set_resolution(double xr, double yr) { _xres = xr; _yres = yr; }

    //! Set the resolution given the length of the longest side (in inches)
    inline void set_resolution_from_size(double size) { _xres = _yres = (_width > _height ? _width : _height) / size; }

    ImageRow* new_row(unsigned int y);

    typedef std::shared_ptr<ImageHeader> ptr;
  }; // class ImageHeader



  //! Image row data
  class ImageRow {
  protected:
    unsigned int _y;
    void *_data;
    omp_lock_t *_lock;

  public:
    //! Constructor
    /*!
      \param y Y coordinate of the row
     */
    ImageRow(unsigned int y);

    //! Constructor
    /*!
      \param y Y coordinate of the row
      \param d Data pointer for row pixels
     */
    ImageRow(unsigned int y, void* d);

    //! Destructor
    ~ImageRow();

    //! Y coordinate of row
    inline unsigned int y(void) const { return _y; }

    //! Set the row pixel data pointer
    inline void set_data(void* d) { _data = d; }

    //! Get the row pixel data pointer
    inline void* data(void) const { return _data; }

    //! Lock this row for exclusive use
    inline void lock(void) { omp_set_lock(_lock); }

    //! Release lock for this row
    inline void unlock(void) { omp_unset_lock(_lock); }

    typedef std::shared_ptr<ImageRow> ptr;
  }; // class ImageRow



  //! Semi-abstract base "role" class for any classes who will receive image data
  class ImageSink : public Worker {
  protected:
    ImageHeader::ptr _sink_header;
    typedef std::list<ImageRow::ptr> _sink_rowqueue_type;
    _sink_rowqueue_type _sink_rowqueue;
    omp_lock_t *_sink_queue_lock;

    //! Lock the queue for exclusive use
    inline void _lock_sink_queue(void) { omp_set_lock(_sink_queue_lock); }

    //! Release lock for the queue
    inline void _unlock_sink_queue(void) { omp_unset_lock(_sink_queue_lock); }

    //! Do something with a row
    virtual void _work_on_row(ImageRow::ptr row) = 0;

  public:
    //! Empty constructor
    ImageSink();

    //! Destructor
    virtual ~ImageSink();

    //! Receive an image header object
    virtual void receive_image_header(ImageHeader::ptr header);

    //! Receive a row of image data
    virtual void receive_image_row(ImageRow::ptr row);

    //! Receive a notification that the image has ended (no data)
    virtual void receive_image_end(void);

    //! Pop a row off of the queue and hand it to _work_on_row()
    virtual void do_work(void);

    typedef std::shared_ptr<ImageSink> ptr;
    typedef std::list<ptr> list;
  }; // class ImageSink



  //! Abstract base "role" class for any classes who will produce image data
  class ImageSource {
  protected:
    ImageSink::list _src_sinks;

    //! Send an image header to all sinks registered with this source
    void _send_image_header(ImageHeader::ptr header);

    //! Send an image row to all sinks registered with this source
    void _send_image_row(ImageRow::ptr row);

    //! Send an image end to all sinks registered with this source
    void _send_image_end(void);

  public:
    ImageSource();

    //! Add a sink to the list
    void add_sink(ImageSink::ptr s);

    //! Add sinks to the list from another list
    void add_sinks(ImageSink::list sl);

    //! Add sinks to the list from another list
    void add_sinks(ImageSink::list::iterator begin, ImageSink::list::iterator end);

    typedef std::shared_ptr<ImageSource> ptr;
  }; // class ImageSource



  //! Abstract base "role" class for any classes that will spawn new image sinks once the header information is received
  class ImageModifier : public ImageSink {
  protected:
    ImageSource::ptr _imgsrc;

  public:
    //! Constructor
    /*!
      \param imgsrc The image source we will attach to as a sink, and to which we will add new sinks
     */
    ImageModifier(ImageSource::ptr imgsrc);

    //! Receive an image header object
    virtual void receive_image_header(ImageHeader::ptr header) = 0;

    typedef std::shared_ptr<ImageModifier> ptr;
  }; // class ImageModifier
}

#endif // __IMAGE_HH__
