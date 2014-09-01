/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////
 

// ZLIB Encoding
//
// The bits of the ClientConnection object to do with zlib.
#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"
#include "libjpeg-turbo-win/jpeglib.h"

static struct jpeg_source_mgr jpegSrcManager;
static JOCTET *jpegBufferPtr;
static size_t jpegBufferLen;
static bool jpegError;
static void JpegInitSource(j_decompress_ptr cinfo);
static boolean JpegFillInputBuffer(j_decompress_ptr cinfo);
static void JpegSkipInputData(j_decompress_ptr cinfo, long num_bytes);
static void JpegTermSource(j_decompress_ptr cinfo);

static void
JpegInitSource(j_decompress_ptr cinfo)
{
  jpegError = false;
}

static boolean
JpegFillInputBuffer(j_decompress_ptr cinfo)
{
  jpegError = true;
  jpegSrcManager.bytes_in_buffer = jpegBufferLen;
  jpegSrcManager.next_input_byte = (JOCTET *)jpegBufferPtr;

  return TRUE;
}

static void
JpegSkipInputData(j_decompress_ptr cinfo, long num_bytes)
{
  if (num_bytes < 0 || (size_t)num_bytes > jpegSrcManager.bytes_in_buffer) {
    jpegError = true;
    jpegSrcManager.bytes_in_buffer = jpegBufferLen;
    jpegSrcManager.next_input_byte = (JOCTET *)jpegBufferPtr;
  } else {
    jpegSrcManager.next_input_byte += (size_t) num_bytes;
    jpegSrcManager.bytes_in_buffer -= (size_t) num_bytes;
  }
}

static void
JpegTermSource(j_decompress_ptr cinfo)
{
  /* No work necessary here. */
}

static void
JpegSetSrcManager(j_decompress_ptr cinfo, char *compressedData, int compressedLen)
{
  jpegBufferPtr = (JOCTET *)compressedData;
  jpegBufferLen = (size_t)compressedLen;

  jpegSrcManager.init_source = JpegInitSource;
  jpegSrcManager.fill_input_buffer = JpegFillInputBuffer;
  jpegSrcManager.skip_input_data = JpegSkipInputData;
  jpegSrcManager.resync_to_restart = jpeg_resync_to_restart;
  jpegSrcManager.term_source = JpegTermSource;
  jpegSrcManager.next_input_byte = jpegBufferPtr;
  jpegSrcManager.bytes_in_buffer = jpegBufferLen;

  cinfo->src = &jpegSrcManager;
}

void 
	ClientConnection::DecompressJpegRect(BYTE *src, int srclen, BYTE *dst, int dst_size,int w, int h,rfbPixelFormat m_remoteformat)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  int pixelsize;
  BYTE *dstBuf=NULL;
  bool dstBufIsTemp = false;

 // Set up JPEG decompression
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  JpegSetSrcManager(&cinfo, (char*)src, srclen);
  jpeg_read_header(&cinfo, TRUE);
  cinfo.out_color_space = JCS_RGB;
  pixelsize = 3;


#ifdef JCS_EXTENSIONS
  // Try to have libjpeg output directly to our native format
  if(m_remoteformat.bitsPerPixel==32) {
    int redShift, greenShift, blueShift;

    if(m_remoteformat.bigEndian) {
      redShift = 24 - m_remoteformat.redShift;
      greenShift = 24 - m_remoteformat.greenShift;
      blueShift = 24 - m_remoteformat.blueShift;
    } else {
      redShift = m_remoteformat.redShift;
      greenShift = m_remoteformat.greenShift;
      blueShift = m_remoteformat.blueShift;
    }

    // libjpeg can only handle some "standard" formats
    if(redShift == 0 && greenShift == 8 && blueShift == 16)
      cinfo.out_color_space = JCS_EXT_RGBX;
    if(redShift == 16 && greenShift == 8 && blueShift == 0)
      cinfo.out_color_space = JCS_EXT_BGRX;
    if(redShift == 24 && greenShift == 16 && blueShift == 8)
      cinfo.out_color_space = JCS_EXT_XBGR;
    if(redShift == 8 && greenShift == 16 && blueShift == 24)
      cinfo.out_color_space = JCS_EXT_XRGB;

    if (cinfo.out_color_space != JCS_RGB) {
      dstBuf = dst;
      pixelsize = 4;
    }
  }
#endif

  if (cinfo.out_color_space == JCS_RGB) {
    dstBuf = new BYTE[w * h * pixelsize];
    dstBufIsTemp = true;
  }

  JSAMPROW *rowPointer = new JSAMPROW[h];
  for (int dy = 0; dy < h; dy++)
    rowPointer[dy] = (JSAMPROW)(&dstBuf[dy * w * pixelsize]);

  jpeg_start_decompress(&cinfo);
  if (cinfo.output_width != (unsigned)w || cinfo.output_height != (unsigned)h ||
      cinfo.output_components != pixelsize) {
      jpeg_destroy_decompress(&cinfo);
      return;
  }

  while (cinfo.output_scanline < cinfo.output_height) {
    jpeg_read_scanlines(&cinfo, &rowPointer[cinfo.output_scanline],
			cinfo.output_height - cinfo.output_scanline);
    if (jpegError) {
      break;
    }
  }

  delete [] rowPointer;

  if (!jpegError) {
    jpeg_finish_decompress(&cinfo);
  }

  jpeg_destroy_decompress(&cinfo);
  if (dstBufIsTemp) delete [] dstBuf;
  return ;
}

void ClientConnection::ReadUltra2Rect(rfbFramebufferUpdateRectHeader *pfburh) {

	UINT numpixels = pfburh->r.w * pfburh->r.h;
    // this assumes at least one byte per pixel. Naughty.
	UINT numRawBytes = numpixels * m_minPixelBytes;
	UINT numCompBytes;
	rfbZlibHeader hdr;
	// Read in the rfbZlibHeader
	omni_mutex_lock l(m_bitmapdcMutex);
	ReadExact((char *)&hdr, sz_rfbZlibHeader);
	numCompBytes = Swap32IfLE(hdr.nBytes);


	// Read in the compressed data
    CheckBufferSize(numCompBytes);
	ReadExact(m_netbuf, numCompBytes);
	CheckZlibBufferSize(numRawBytes);

	DecompressJpegRect((BYTE*)m_netbuf,numCompBytes, (BYTE*)m_zlibbuf, numRawBytes, pfburh->r.w, pfburh->r.h,m_myFormat);

	SoftCursorLockArea(pfburh->r.x, pfburh->r.y,pfburh->r.w,pfburh->r.h);
	if (!Check_Rectangle_borders(pfburh->r.x, pfburh->r.y,pfburh->r.w,pfburh->r.h)) return;
	if (m_DIBbits) ConvertAll(pfburh->r.w,pfburh->r.h,pfburh->r.x, pfburh->r.y,m_myFormat.bitsPerPixel/8,(BYTE *)m_zlibbuf,(BYTE *)m_DIBbits,m_si.framebufferWidth);

}