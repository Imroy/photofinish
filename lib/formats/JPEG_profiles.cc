/*
	Copyright 2014 Ian Tester

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
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <jpeglib.h>
#include "CMS.hh"
#include "Destination.hh"

namespace PhotoFinish {

  CMS::Profile::ptr jpeg_read_profile(jpeg_decompress_struct* dinfo, Destination::ptr dest) {
    unsigned int profile_size = 0;
    unsigned char num_markers = 0;
    std::map<unsigned char, jpeg_marker_struct*> icc_markers;
    for (jpeg_marker_struct *marker = dinfo->marker_list; marker != nullptr; marker = marker->next)
      if ((marker->marker == JPEG_APP0 + 2)
	  && (marker->data_length > 14)
	  && (memcmp(marker->data, "ICC_PROFILE\0", 12) == 0)) {

	profile_size += marker->data_length - 14;
	unsigned char i = *(marker->data + 12) - 1;
	icc_markers[i] = marker;

	unsigned char j = *(marker->data + 13);
	if ((i > 0) && (j != num_markers))
	  std::cerr << "** Got a different number of markers! (" << j << " != " << num_markers << ") **" << std::endl;
	num_markers = j;
      }

    if (profile_size == 0)	// Probably no APP2 markers
      return std::make_shared<CMS::Profile>();

    if (num_markers != icc_markers.size()) {
      std::cerr << "** Supposed to have " << (int)num_markers << " APP2 markers, but only have " << icc_markers.size() << " in list **" << std::endl;
      return std::make_shared<CMS::Profile>();
    }

    unsigned char *profile_data = new unsigned char[profile_size];
    unsigned char *pos = profile_data;
    for (unsigned int i = 0; i < num_markers; i++) {
      memcpy(pos, icc_markers[i]->data + 14, icc_markers[i]->data_length - 14);
      pos += icc_markers[i]->data_length - 14;
    }

    CMS::Profile::ptr profile = std::make_shared<CMS::Profile>(profile_data, profile_size);
    if (profile) {
      std::string profile_name = profile->description("en", "");
      if (profile_name.length() > 0)
	dest->set_profile(profile_name, profile_data, profile_size);
      else
	dest->set_profile("JPEG APP2", profile_data, profile_size);
      std::cerr << "\tRead embedded profile \"" << dest->profile()->name().get() << "\" (" << profile_size << " bytes in " << (int)num_markers << " APP2 markers)" << std::endl;
    }

    return profile;
  }

  void jpeg_write_profile(jpeg_compress_struct* cinfo, unsigned char *data, unsigned int size) {
    unsigned char num_markers = ceil(size / 65519.0); // max 65533 bytes in a marker, minus 14 bytes for the ICC overhead
    std::cerr << "\tEmbedding profile from data (" << size << " bytes, " << (int)num_markers << " markers)." << std::endl;

    unsigned int data_left = size;
    for (unsigned char i = 0; i < num_markers; i++) {
      int marker_data_size = data_left > 65519 ? 65519 : data_left;
      JOCTET *APP2 = new JOCTET[marker_data_size + 14];
      memcpy(APP2, "ICC_PROFILE\0", 12);
      APP2[12] = i + 1;
      APP2[13] = num_markers;
      memcpy(APP2 + 14, data, marker_data_size);

      jpeg_write_marker(cinfo, JPEG_APP0 + 2, APP2, marker_data_size + 14);
      data += marker_data_size;
      data_left -= marker_data_size;
    }
  }

}
