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
#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "Image.hh"

namespace PhotoFinish {

  ImageHeader::ImageHeader() :
    _width(-1),
    _height(-1)
  {}

  ImageHeader::ImageHeader(unsigned int w, unsigned int h) :
    _width(w),
    _height(h)
  {}

  ImageRow* ImageHeader::new_row(unsigned int y) {
    unsigned char bytes = T_BYTES(_cmsType);
    if (T_FLOAT(_cmsType) && (bytes == 0)) // Double-precision floating-point format
      bytes = 8;
    void *data = malloc((T_CHANNELS(_cmsType) + T_EXTRA(_cmsType)) * bytes);
    return new ImageRow(y, data);
  }



  ImageRow::ImageRow(unsigned int y) :
    _y(y),
    _data(NULL),
    _lock((omp_lock_t*)malloc(sizeof(omp_lock_t)))
  {
    omp_init_lock(_lock);
  }

  ImageRow::ImageRow(unsigned int y, void* d) :
    _y(y),
    _data(d),
    _lock((omp_lock_t*)malloc(sizeof(omp_lock_t)))
  {
    omp_init_lock(_lock);
  }

  ImageRow::~ImageRow() {
    if (_data != NULL) {
      free(_data);
      _data = NULL;
    }
    omp_destroy_lock(_lock);
    free(_lock);
  }



  ImageSink::ImageSink() :
    _sink_queue_lock((omp_lock_t*)malloc(sizeof(omp_lock_t)))
  {
    omp_init_lock(_sink_queue_lock);
  }

  ImageSink::~ImageSink() {
    omp_destroy_lock(_sink_queue_lock);
    free(_sink_queue_lock);
  }

  void ImageSink::receive_image_header(ImageHeader::ptr header) {
    _sink_header = header;
  }

  void ImageSink::receive_image_row(ImageRow::ptr row) {
    this->_lock_sink_queue();
    _sink_rowqueue.push_back(row);
    this->_unlock_sink_queue();
  }

  void ImageSink::receive_image_end(void) {
    _work_finished = true;
  }

  void ImageSink::do_work(void) {
    this->_lock_sink_queue();
    ImageRow::ptr row = _sink_rowqueue.front();
    _sink_rowqueue.pop_front();
    this->_unlock_sink_queue();
    this->_work_on_row(row);
  }



  ImageSource::ImageSource() {
  }

  void ImageSource::_send_image_header(ImageHeader::ptr header) {
    for (ImageSink::list::iterator ri = _src_sinks.begin(); ri != _src_sinks.end(); ri++)
      (*ri)->receive_image_header(header);
  }

  void ImageSource::_send_image_row(ImageRow::ptr row) {
    for (ImageSink::list::iterator ri = _src_sinks.begin(); ri != _src_sinks.end(); ri++)
      (*ri)->receive_image_row(row);
  }

  void ImageSource::_send_image_end(void) {
    for (ImageSink::list::iterator ri = _src_sinks.begin(); ri != _src_sinks.end(); ri++)
      (*ri)->receive_image_end();
  }

  void ImageSource::add_sink(ImageSink* s) {
    _src_sinks.push_back(ImageSink::ptr(s));
  }

  void ImageSource::add_sinks(ImageSink::list sl) {
    for (ImageSink::list::iterator si = sl.begin(); si != sl.end(); si++)
      _src_sinks.push_back(*si);
  }

  void ImageSource::add_sinks(ImageSink::list::iterator begin, ImageSink::list::iterator end) {
    std::copy(begin, end, _src_sinks.end());
  }

}
