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
#include <omp.h>
#include "Image.hh"

namespace PhotoFinish {

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



  ImageHeader::ImageHeader() :
    _width(-1),
    _height(-1),
    _profile(NULL),
    _cmsType(0),
    _row_size(0)
  {}

  ImageHeader::ImageHeader(unsigned int w, unsigned int h) :
    _width(w),
    _height(h),
    _profile(NULL),
    _cmsType(0),
    _row_size(0)
  {
    unsigned char bytes = T_BYTES(_cmsType);
    if (T_FLOAT(_cmsType) && (bytes == 0)) // Double-precision floating-point format
      bytes = 8;
    _row_size = _width * (T_CHANNELS(_cmsType) + T_EXTRA(_cmsType)) * bytes;
  }

  ImageRow::ptr ImageHeader::new_row(unsigned int y) {
    void *data = malloc(_row_size);
    memset(data, 0, _row_size);
    return ImageRow::ptr(new ImageRow(y, data));
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
    for (std::list<end_handler>::iterator ehi = _end_handlers.begin(); ehi != _end_handlers.end(); ehi++)
      (*ehi)();
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
    for (std::list<header_handler>::iterator hhi = _header_handlers.begin(); hhi != _header_handlers.end(); hhi++)
      (*hhi)(header);

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

  void ImageSource::add_sink(ImageSink::ptr s) {
    _src_sinks.push_back(s);
  }

  void ImageSource::add_sinks(ImageSink::list sl) {
    for (ImageSink::list::iterator si = sl.begin(); si != sl.end(); si++)
      _src_sinks.push_back(*si);
  }

  void ImageSource::add_sinks(ImageSink::list::iterator begin, ImageSink::list::iterator end) {
    std::copy(begin, end, _src_sinks.end());
  }

}
