/****************************************************************************
* JPEG 6b Memory Source
*
* This is an additional memory source manager for libJPEG6b
* Based entirely on jdatasrc.c
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*** JPEG includes ***/
#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"

/* Expanded data source object for memory input */

typedef struct {
struct jpeg_source_mgr pub; /* public fields */

JOCTET eoi_buffer[2]; /* a place to put a dummy EOI */
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;


/*
* Initialize source --- called by jpeg_read_header
* before any data is actually read.
*/

METHODDEF(void)
init_source (j_decompress_ptr cinfo)
{
/* No work, since jpeg_memory_src set up the buffer pointer and count.
* Indeed, if we want to read multiple JPEG images from one buffer,
* this *must* not do anything to the pointer.
*/
}


/*
* Fill the input buffer --- called whenever buffer is emptied.
*
* In this application, this routine should never be called; if it is called,
* the decompressor has overrun the end of the input buffer, implying we
* supplied an incomplete or corrupt JPEG datastream. A simple error exit
* might be the most appropriate response.
*
* But what we choose to do in this code is to supply dummy EOI markers
* in order to force the decompressor to finish processing and supply
* some sort of output image, no matter how corrupted.
*/

METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
		
my_src_ptr src = (my_src_ptr) cinfo->src;

WARNMS(cinfo, JWRN_JPEG_EOF);

/* Create a fake EOI marker */
src->eoi_buffer[0] = (JOCTET) 0xFF;
src->eoi_buffer[1] = (JOCTET) JPEG_EOI;
src->pub.next_input_byte = src->eoi_buffer;
src->pub.bytes_in_buffer = 2;

return TRUE;
}


/*
* Skip data --- used to skip over a potentially large amount of
* uninteresting data (such as an APPn marker).
*
* If we overrun the end of the buffer, we let fill_input_buffer deal with
* it. An extremely large skip could cause some time-wasting here, but
* it really isn't supposed to happen ... and the decompressor will never
* skip more than 64K anyway.
*/

METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
my_src_ptr src = (my_src_ptr) cinfo->src;

if (num_bytes > 0) {
while (num_bytes > (long) src->pub.bytes_in_buffer) {
num_bytes -= (long) src->pub.bytes_in_buffer;
(void) fill_input_buffer(cinfo);
/* note we assume that fill_input_buffer will never return FALSE,
* so suspension need not be handled.
*/
}
src->pub.next_input_byte += (size_t) num_bytes;
src->pub.bytes_in_buffer -= (size_t) num_bytes;
}
}


/*
* An additional method that can be provided by data source modules is the
* resync_to_restart method for error recovery in the presence of RST markers.
* For the moment, this source module just uses the default resync method
* provided by the JPEG library. That method assumes that no backtracking
* is possible.
*/


/*
* Terminate source --- called by jpeg_finish_decompress
* after all data has been read. Often a no-op.
*
* NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
* application must deal with any cleanup that should happen even
* for error exit.
*/

METHODDEF(void)
term_source (j_decompress_ptr cinfo)
{
/* no work necessary here */
}


/*
* Prepare for input from a memory buffer.
*/

GLOBAL(void)
jpeg_memory_src (j_decompress_ptr cinfo, const JOCTET * buffer, size_t bufsize)
{
my_src_ptr src;

/* The source object is made permanent so that a series of JPEG images
* can be read from a single buffer by calling jpeg_memory_src
* only before the first one.
* This makes it unsafe to use this manager and a different source
* manager serially with the same JPEG object. Caveat programmer.
*/
if (cinfo->src == NULL) { /* first time for this JPEG object? */
cinfo->src = (struct jpeg_source_mgr *)
(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
SIZEOF(my_source_mgr));
}

src = (my_src_ptr) cinfo->src;
src->pub.init_source = init_source;
src->pub.fill_input_buffer = fill_input_buffer;
src->pub.skip_input_data = skip_input_data;
src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
src->pub.term_source = term_source;

src->pub.next_input_byte = buffer;
src->pub.bytes_in_buffer = bufsize;
}

