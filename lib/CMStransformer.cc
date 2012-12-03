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
#include "CMStransformer.hh"

namespace PhotoFinish {

  CMStransformer::CMStransformer(cmsHPROFILE dest_profile, cmsUInt32Number dest_cmsType, cmsUInt32Number intent) :
    ImageFilter(),
    _profile(dest_profile),
    _cmsType(dest_cmsType),
    _intent(intent),
    _transform(NULL)
  {
  }

  CMStransformer::~CMStransformer() {
    if (_transform != NULL)
      cmsDeleteTransform(_transform);
  }

  void CMStransformer::receive_image_header(ImageHeader::ptr header) {
    ImageSink::receive_image_header(header);

    if (_transform != NULL)
      cmsDeleteTransform(_transform);

    _transform = cmsCreateTransform(header->profile(),
				    header->cmsType(),
				    _profile,
				    _cmsType,
				    _intent,
				    0);

    _header = ImageHeader::ptr(new ImageHeader(header->width(), header->height(), _cmsType));
    _header->set_profile(_profile);
    this->_send_image_header(_header);
  }

  void CMStransformer::receive_image_end(void) {
    this->_send_image_end();
  }

  void CMStransformer::_work_on_row(ImageRow::ptr row) {
    ImageRow::ptr outrow = _header->new_row(row->y());

    row->lock();
    cmsDoTransform(_transform, row->data(), outrow->data(), _header->width());
    row->unlock();

    this->_send_image_row(outrow);
  }

} // namespace PhotoFinish
