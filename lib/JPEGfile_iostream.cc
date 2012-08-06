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

  //! Structure holding information for the istream reader
  struct jpeg_source_state_t {
    JOCTET *buffer;
    std::istream *is;
    size_t buffer_size;
  };

  void jpeg_istream_init_source(j_decompress_ptr cinfo) {
    jpeg_source_mgr *smgr = (jpeg_source_mgr*)(cinfo->src);
    jpeg_source_state_t *ss = (jpeg_source_state_t*)(cinfo->client_data);
    ss->is->seekg(0, std::ios_base::beg);
    ss->buffer = (JOCTET*)malloc(ss->buffer_size);
    if (ss->buffer == NULL)
      throw MemAllocError("Out of memory?");
    smgr->bytes_in_buffer = 0;
  }

  boolean jpeg_istream_fill_input_buffer(j_decompress_ptr cinfo) {
    jpeg_source_mgr *smgr = (jpeg_source_mgr*)(cinfo->src);
    jpeg_source_state_t *ss = (jpeg_source_state_t*)(cinfo->client_data);

    ss->is->read((char*)ss->buffer, ss->buffer_size);
    smgr->next_input_byte = ss->buffer;
    smgr->bytes_in_buffer = ss->is->gcount();

    return TRUE;
  }

  void jpeg_istream_skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
    if (num_bytes < 1)
      return;
    jpeg_source_mgr *smgr = (jpeg_source_mgr*)(cinfo->src);
    jpeg_source_state_t *ss = (jpeg_source_state_t*)(cinfo->client_data);

    if ((unsigned long)num_bytes > smgr->bytes_in_buffer) {
      ss->is->seekg(num_bytes - smgr->bytes_in_buffer, std::ios_base::cur);
      smgr->next_input_byte += smgr->bytes_in_buffer; // necessary?
      smgr->bytes_in_buffer = 0;
    } else {
      smgr->next_input_byte += num_bytes;
      smgr->bytes_in_buffer -= num_bytes;
    }
  }

  boolean jpeg_istream_resync_to_restart(j_decompress_ptr cinfo, int desired) {
    return jpeg_resync_to_restart(cinfo, desired);
  }

  void jpeg_istream_term_source(j_decompress_ptr cinfo) {
    jpeg_source_state_t *ss = (jpeg_source_state_t*)(cinfo->client_data);
    free(ss->buffer);
  }

  void jpeg_istream_src(j_decompress_ptr cinfo, std::istream* is) {
    jpeg_source_state_t *ss = (jpeg_source_state_t*)malloc(sizeof(jpeg_source_state_t));
    ss->is = is;
    ss->buffer_size = 1048576;
    cinfo->client_data = (void*)ss;

    jpeg_source_mgr *smgr = (jpeg_source_mgr*)malloc(sizeof(jpeg_source_mgr));
    smgr->init_source = jpeg_istream_init_source;
    smgr->fill_input_buffer = jpeg_istream_fill_input_buffer;
    smgr->skip_input_data = jpeg_istream_skip_input_data;
    smgr->resync_to_restart = jpeg_istream_resync_to_restart;
    smgr->term_source = jpeg_istream_term_source;
    cinfo->src = smgr;
  }

  void jpeg_istream_src_free(j_decompress_ptr cinfo) {
    free((jpeg_source_state_t*)cinfo->client_data);
    free(cinfo->src);
  }


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
