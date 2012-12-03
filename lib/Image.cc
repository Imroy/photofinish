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
    omp_set_lock(_lock);
    if (_data != NULL) {
      free(_data);
      _data = NULL;
    }
    omp_unset_lock(_lock);
    omp_destroy_lock(_lock);
    free(_lock);
  }



  ImageHeader::ImageHeader(unsigned int w, unsigned int h, cmsUInt32Number t) :
    _width(w),
    _height(h),
    _profile(NULL),
    _cmsType(t),
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
    Worker()
  {
    omp_init_lock(&_sink_queue_lock);
  }

  ImageSink::~ImageSink() {
    omp_destroy_lock(&_sink_queue_lock);
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
  }



  ImageSource::ImageSource() {
    std::cerr << "ImageSource: Have " << _header_hooks.size() << " header handler(s)..." << std::endl;
  }

  void ImageSource::_send_image_header(ImageHeader::ptr header) {
    std::cerr << "ImageSource: Sending image header to " << _header_hooks.size() << " header handler(s)..." << std::endl;
    for (std::list<header_hook>::iterator hhi = _header_hooks.begin(); hhi != _header_hooks.end(); hhi++)
      (*hhi)(header);

    std::cerr << "ImageSource: Sending image header to " << _src_sinks.size() << " sink(s)..." << std::endl;
    for (ImageSink::list::iterator ri = _src_sinks.begin(); ri != _src_sinks.end(); ri++)
      (*ri)->receive_image_header(header);
  }

  void ImageSource::_send_image_row(ImageRow::ptr row) {
    //    std::cerr << "ImageSource: Sending image row to " << _src_sinks.size() << " sink(s)..." << std::endl;
    for (ImageSink::list::iterator ri = _src_sinks.begin(); ri != _src_sinks.end(); ri++)
      (*ri)->receive_image_row(row);
  }

  void ImageSource::_send_image_end(void) {
    //    std::cerr << "ImageSource: Sending image end to " << _src_sinks.size() << " sink(s)..." << std::endl;
    for (ImageSink::list::iterator ri = _src_sinks.begin(); ri != _src_sinks.end(); ri++)
      (*ri)->receive_image_end();
  }

  void ImageSource::add_header_hook(const header_hook& hh) {
    std::cerr << "ImageSource: Have " << _header_hooks.size() << " header handler(s)..." << std::endl;
    _header_hooks.push_back(hh);
    std::cerr << "ImageSource: Have " << _header_hooks.size() << " header handler(s)..." << std::endl;
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



  ImageFilter::ImageFilter() :
    ImageSink(),
    ImageSource()
  {}

  ImageFilter::~ImageFilter() {
  }

  void ImageFilter::do_work(void) {
    this->_lock_sink_queue();
    if (_sink_rowqueue.size() > 0) {
      ImageRow::ptr row = _sink_rowqueue.front();
      this->_work_on_row(row);
      _sink_rowqueue.pop_front();
    }
    this->_unlock_sink_queue();
  }

}
