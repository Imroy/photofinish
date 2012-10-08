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
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <jpeglib.h>
#include <lcms2.h>

namespace PhotoFinish {

  cmsHPROFILE jpeg_read_profile(jpeg_decompress_struct* dinfo) {
    unsigned int profile_size = 0;
    unsigned char num_markers = 0;
    std::map<unsigned char, jpeg_marker_struct*> app2_markers;
    for (jpeg_marker_struct *marker = dinfo->marker_list; marker != NULL; marker = marker->next)
      if ((marker->marker == JPEG_APP0 + 2)
	  && (marker->data_length > 14)
	  && (strncmp((char*)marker->data, "ICC_PROFILE", 11) == 0)) {

	profile_size += marker->data_length - 14;
	unsigned char i = *(marker->data + 12) - 1;
	app2_markers[i] = marker;

	unsigned char j = *(marker->data + 13);
	if ((i > 0) && (j != num_markers))
	  std::cerr << "** Got a different number of markers! (" << j << " != " << num_markers << ") **" << std::endl;
	num_markers = j;
      }

    if (profile_size == 0)	// Probably no APP2 markers
      return NULL;

    if (num_markers != app2_markers.size()) {
      std::cerr << "** Supposed to have " << (int)num_markers << " APP2 markers, but only have " << app2_markers.size() << " in list **" << std::endl;
      return NULL;
    }

    unsigned char *profile_data = (unsigned char*)malloc(profile_size);
    unsigned char *pos = profile_data;
    for (unsigned int i = 0; i < num_markers; i++) {
      memcpy(pos, app2_markers[i]->data + 14, app2_markers[i]->data_length - 14);
      pos += app2_markers[i]->data_length - 14;
    }

    cmsHPROFILE profile = cmsOpenProfileFromMem(profile_data, profile_size);
    return profile;
  }

  void jpeg_write_profile(jpeg_compress_struct* cinfo, unsigned char *data, unsigned int size) {
    unsigned char num_markers = ceil(size / 65519.0); // max 65533 bytes in a marker, minus 14 bytes for the ICC overhead
    std::cerr << "\tEmbedding profile from data (" << size << " bytes, " << (int)num_markers << " markers)." << std::endl;
    unsigned int data_left = size;
    for (unsigned char i = 0; i < num_markers; i++) {
      int marker_data_size = data_left > 65519 ? 65519 : data_left;
      JOCTET *APP2 = (JOCTET*)malloc(marker_data_size + 14);
      strcpy((char*)APP2, "ICC_PROFILE");
      APP2[12] = i + 1;
      APP2[13] = num_markers;
      memcpy(APP2 + 14, data, marker_data_size);
      jpeg_write_marker(cinfo, JPEG_APP0 + 2, APP2, marker_data_size + 14);
      data += marker_data_size;
    }
  }

}
