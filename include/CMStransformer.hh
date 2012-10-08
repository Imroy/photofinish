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
#ifndef __CMSTRANSFORMER_HH__
#define __CMSTRANSFORMER_HH__

#include <lcms2.h>
#include "Image.hh"

namespace PhotoFinish {

  class CMStransformer : public ImageSink, public ImageSource {
  private:
    cmsHPROFILE _profile;
    cmsUInt32Number _cmsType;
    cmsUInt32Number _intent;
    cmsHTRANSFORM _transform;
    ImageHeader::ptr _header;

    virtual void _work_on_row(ImageRow::ptr row);

  public:
    //! Constructor
    CMStransformer(cmsHPROFILE dest_profile, cmsUInt32Number dest_cmsType, cmsUInt32Number intent);

    ~CMStransformer();

    virtual void receive_image_header(ImageHeader::ptr header);

    virtual void receive_image_end(void);
  }; // class CMStransformer

} // namespace PhotoFinish

#endif // __CMSTRANSFORMER_HH__
