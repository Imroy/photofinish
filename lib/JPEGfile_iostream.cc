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
#include "JPEGfile_iostream.hh"
#include "Exception.hh"

namespace PhotoFinish {

  //! Structure holding information for the ostream writer
  struct jpeg_destination_state_t {
    JOCTET *buffer;
    std::ostream *os;
    size_t buffer_size;
  };

  //! Called by libJPEG to initialise the "destination manager"
  static void jpeg_ostream_init_destination(j_compress_ptr cinfo) {
    jpeg_destination_mgr *dmgr = (jpeg_destination_mgr*)(cinfo->dest);
    jpeg_destination_state_t *ds = (jpeg_destination_state_t*)(cinfo->client_data);
    ds->buffer = (JOCTET*)malloc(ds->buffer_size);
    if (ds->buffer == NULL)
      throw MemAllocError("Out of memory?");

    dmgr->next_output_byte = ds->buffer;
    dmgr->free_in_buffer = ds->buffer_size;
  }

  //! Called by libJPEG to write the output buffer and prepare it for more data
  static boolean jpeg_ostream_empty_output_buffer(j_compress_ptr cinfo) {
    jpeg_destination_mgr *dmgr = (jpeg_destination_mgr*)(cinfo->dest);
    jpeg_destination_state_t *ds = (jpeg_destination_state_t*)(cinfo->client_data);
    ds->os->write((char*)ds->buffer, ds->buffer_size);
    dmgr->next_output_byte = ds->buffer;
    dmgr->free_in_buffer = ds->buffer_size;
    return 1;
  }

  //! Called by libJPEG to write any remaining data in the output buffer and deallocate it
  static void jpeg_ostream_term_destination(j_compress_ptr cinfo) {
    jpeg_destination_mgr *dmgr = (jpeg_destination_mgr*)(cinfo->dest);
    jpeg_destination_state_t *ds = (jpeg_destination_state_t*)(cinfo->client_data);
    ds->os->write((char*)ds->buffer, ds->buffer_size - dmgr->free_in_buffer);
    free(ds->buffer);
    ds->buffer = NULL;
    dmgr->free_in_buffer = 0;
  }

  void jpeg_ostream_dest(j_compress_ptr cinfo, std::ostream* os) {
    jpeg_destination_state_t *ds = (jpeg_destination_state_t*)malloc(sizeof(jpeg_destination_state_t));
    ds->os = os;
    ds->buffer_size = 1048576;
    cinfo->client_data = (void*)ds;

    jpeg_destination_mgr *dmgr = (jpeg_destination_mgr*)malloc(sizeof(jpeg_destination_mgr));
    dmgr->init_destination = jpeg_ostream_init_destination;
    dmgr->empty_output_buffer = jpeg_ostream_empty_output_buffer;
    dmgr->term_destination = jpeg_ostream_term_destination;
    cinfo->dest = dmgr;
  }

  void jpeg_ostream_dest_free(j_compress_ptr cinfo) {
    free((jpeg_destination_state_t*)cinfo->client_data);
    free(cinfo->dest);
  }

}
