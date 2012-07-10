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
#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>

namespace PhotoFinish {

  void jpegfile_scan_RGB(jpeg_compress_struct* cinfo) {
    std::cerr << "Image has " << cinfo->num_components << " components." << std::endl;
    jpeg_scan_info *my_scan_info = (jpeg_scan_info*)malloc(11 * sizeof(jpeg_scan_info));

    // Luma DC
    my_scan_info[0] = {
      comps_in_scan: 1,
      component_index: { 0 },
      Ss: 0, Se: 0,
      Ah: 0, Al: 0,
    };

    // Chroma DC
    my_scan_info[1] = {
      comps_in_scan: 2,
      component_index: { 1, 2 },
      Ss: 0, Se: 0,
      Ah: 0, Al: 0,
    };

    // Cb, Cr AC scans
    my_scan_info[2] = {
      comps_in_scan: 1,
      component_index: { 2 },
      Ss: 1, Se: 9,
      Ah: 0, Al: 1,
    };
    my_scan_info[3] = {
      comps_in_scan: 1,
      component_index: { 1 },
      Ss: 1, Se: 9,
      Ah: 0, Al: 1,
    };

    // Some luma AC
    my_scan_info[4] = {
      comps_in_scan: 1,
      component_index: { 0 },
      Ss: 1, Se: 5,
      Ah: 0, Al: 1,
    };

    // Rest of the Cb, Cr AC scans
    my_scan_info[5] = {
      comps_in_scan: 1,
      component_index: { 2 },
      Ss: 10, Se: 63,
      Ah: 0, Al: 1,
    };
    my_scan_info[6] = {
      comps_in_scan: 1,
      component_index: { 1 },
      Ss: 10, Se: 63,
      Ah: 0, Al: 1,
    };

    // Rest of the luma AC
    my_scan_info[7] = {
      comps_in_scan: 1,
      component_index: { 0 },
      Ss: 6, Se: 63,
      Ah: 0, Al: 1,
    };

    // remaining lsb bits of the AC scans
    my_scan_info[8] = {
      comps_in_scan: 1,
      component_index: { 0 },
      Ss: 1, Se: 63,
      Ah: 1, Al: 0,
    };
    my_scan_info[9] = {
      comps_in_scan: 1,
      component_index: { 2 },
      Ss: 1, Se: 63,
      Ah: 1, Al: 0,
    };
    my_scan_info[10] = {
      comps_in_scan: 1,
      component_index: { 1 },
      Ss: 1, Se: 63,
      Ah: 1, Al: 0,
    };

    cinfo->num_scans = 11;
    cinfo->scan_info = my_scan_info;
  }

  void jpegfile_scan_greyscale(jpeg_compress_struct* cinfo) {
    jpeg_scan_info *my_scan_info = (jpeg_scan_info*)malloc(5 * sizeof(jpeg_scan_info));
      // Y DC
      my_scan_info[0] = {
	comps_in_scan: 1,
	component_index: { 0 },
	Ss: 0, Se: 0,
	Ah: 0, Al: 0,
      };

      // Some Y AC
      my_scan_info[1] = {
	comps_in_scan: 1,
	component_index: { 0 },
	Ss: 1, Se: 5,
	Ah: 0, Al: 2,
      };

      // Rest of the Y AC
      my_scan_info[2] = {
	comps_in_scan: 1,
	component_index: { 0 },
	Ss: 6, Se: 63,
	Ah: 0, Al: 2,
      };

      // remaining lsb bits of the AC scans
      my_scan_info[3] = {
	comps_in_scan: 1,
	component_index: { 0 },
	Ss: 1, Se: 63,
	Ah: 2, Al: 1,
      };
      my_scan_info[4] = {
	comps_in_scan: 1,
	component_index: { 0 },
	Ss: 1, Se: 63,
	Ah: 1, Al: 0,
      };

    cinfo->num_scans = 5;
    cinfo->scan_info = my_scan_info;
  }

}
