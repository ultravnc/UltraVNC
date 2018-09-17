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
#include "stdhdrs.h"
#include "vncEncodeUltra2.h"
#include <mmsystem.h>

struct jpeg_destination_mgr jpegDstManager;
static JOCTET *jpegDstBuffer;
static size_t jpegDstBufferLen;
static void JpegInitDestination(j_compress_ptr cinfo);
static boolean JpegEmptyOutputBuffer(j_compress_ptr cinfo);
static void JpegTermDestination(j_compress_ptr cinfo);
static void JpegSetDstManager(j_compress_ptr cinfo, JOCTET *buf, size_t buflen);
static bool jpegError;
static int jpegDstDataLen;

#define IN_LEN		(128*1024)
#define OUT_LEN		(IN_LEN + IN_LEN / 64 + 16 + 3)
#define HEAP_ALLOC(var,size) \
	lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]
static HEAP_ALLOC(wrkmem,LZO1X_1_MEM_COMPRESS);


vncEncodeUltra2::vncEncodeUltra2()
{
	m_buffer = NULL;
	m_bufflen = 0;
	destbuffer=NULL;
	m_quality = 0;
	m_rowPointer = NULL;
	m_rowPointerSize = 0;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
}

vncEncodeUltra2::~vncEncodeUltra2()
{
	if (m_buffer != NULL)
		delete [] m_buffer;
	if (destbuffer!=0) 
		free (destbuffer);
	if (m_rowPointer != NULL)
		delete[] m_rowPointer;
	jpeg_destroy_compress(&cinfo);
}

void
vncEncodeUltra2::Init()
{
	encodedSize=0;
	rectangleOverhead=0;
	transmittedSize=0;
	dataSize=0;
	vncEncoder::Init();
}

UINT
vncEncodeUltra2::RequiredBuffSize(UINT width, UINT height)
{
	int result;

	// The Ultra library specifies a maximum compressed size of
	// the raw size plus one percent plus 8 bytes.  We also need
	// to cover the Ultra header space.
	result = vncEncoder::RequiredBuffSize(width, height);
	result += result/ 64 + 16 + 3 + sz_rfbZlibHeader+sz_rfbFramebufferUpdateRectHeader+ sz_rfbZlibHeader;
	return result;
}


UINT
vncEncodeUltra2::NumCodedRects(const rfb::Rect &rect)
{
	return 0;
}

/*****************************************************************************
 *
 * Routines to implement Ultra Encoding (LZ+Huffman compression) by calling
 * the included Ultra library.
 */

// Encode the rectangle using Ultra compression
// Encode the rectangle using Ultra compression
inline UINT
vncEncodeUltra2::EncodeRect(BYTE *source, VSocket *outConn, BYTE *dest, const rfb::Rect &rect)
{

	int size = 0;
	const int rectW = rect.br.x - rect.tl.x;
	const int rectH = rect.br.y - rect.tl.y;
	const int rawDataSize = (rectW*rectH*m_remoteformat.bitsPerPixel / 8);

	if (rectW == 0 || rectH == 0) 
		return 0;

	rfbFramebufferUpdateRectHeader *surh=(rfbFramebufferUpdateRectHeader *)dest;
	// Modif rdv@2002 - v1.1.x - Application Resize
	surh->r.x = (CARD16) rect.tl.x-monitor_Offsetx;
	surh->r.y = (CARD16) rect.tl.y-monitor_Offsety;
	surh->r.w = (CARD16) (rectW);
	surh->r.h = (CARD16) (rectH);
	surh->r.x = Swap16IfLE(surh->r.x);
	surh->r.y = Swap16IfLE(surh->r.y);
	surh->r.w = Swap16IfLE(surh->r.w);
	surh->r.h = Swap16IfLE(surh->r.h);
	surh->encoding = Swap32IfLE(rfbEncodingUltra2);

	// create a space big enough for the Jpeg encoded pixels
	if (m_bufflen < rawDataSize + 1000) {
		if (m_buffer != NULL) {
			delete [] m_buffer;
			m_buffer = NULL;
		}
		m_buffer = new BYTE [rawDataSize+1000];
		if (m_buffer == NULL)
			return vncEncoder::EncodeRect(source, dest, rect);
		m_bufflen = rawDataSize+999;
	}
	// Translate the data into our new buffer
	Translate(source, m_buffer, rect);
	rfbZlibHeader *zlibh=(rfbZlibHeader *)(dest+sz_rfbFramebufferUpdateRectHeader);
	size=0;
	if (rectW >= 8 && rectH >= 8) {
		size=SendJpegRect(m_buffer,dest+sz_rfbFramebufferUpdateRectHeader+sz_rfbZlibHeader, rawDataSize + 1000, rectW , rectH , m_qualitylevel*10,m_remoteformat);		
		zlibh->nBytes = Swap32IfLE(size);
	}
	// jpeg failed
	if (size == 0) {		
		if (rawDataSize < 64)
			return vncEncoder::EncodeRect(source, dest, rect);
				
		if (lzo==false && lzo_init() == LZO_E_OK)
			lzo=true;
		lzo1x_1_compress(m_buffer,rawDataSize,dest+sz_rfbFramebufferUpdateRectHeader+sz_rfbZlibHeader,&out_len,wrkmem);
		if (out_len > (lzo_uint)rawDataSize)
			return vncEncoder::EncodeRect(source, dest, rect);
		surh->encoding = Swap32IfLE(rfbEncodingUltra);
		zlibh->nBytes = Swap32IfLE(out_len);
		size=out_len;
	}
	transmittedSize += sz_rfbFramebufferUpdateRectHeader + sz_rfbZlibHeader + size;
	return sz_rfbFramebufferUpdateRectHeader + sz_rfbZlibHeader + size;
}

void vncEncodeUltra2::checkRowPointer(int h)
{
	if (h > m_rowPointerSize) {
		if (m_rowPointer != NULL)
			delete[] m_rowPointer;
		m_rowPointer = new JSAMPROW[h];
		m_rowPointerSize = h;
	}
}

int
vncEncodeUltra2::SendJpegRect(BYTE *src,BYTE *dst, int dst_size, int w, int h, int quality,rfbPixelFormat m_remoteformat)
{
  BYTE *srcBuf=NULL;

  cinfo.image_width = w;
  cinfo.image_height = h;
  cinfo.in_color_space = JCS_RGB;
  cinfo.input_components = 3;

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

    if(redShift == 0 && greenShift == 8 && blueShift == 16)
      cinfo.in_color_space = JCS_EXT_RGBX;
    if(redShift == 16 && greenShift == 8 && blueShift == 0)
      cinfo.in_color_space = JCS_EXT_BGRX;
    if(redShift == 24 && greenShift == 16 && blueShift == 8)
      cinfo.in_color_space = JCS_EXT_XBGR;
    if(redShift == 8 && greenShift == 16 && blueShift == 24)
      cinfo.in_color_space = JCS_EXT_XRGB;

    if (cinfo.in_color_space != JCS_RGB) {
      srcBuf = src;
      cinfo.input_components = 4;
    }
  }

  if (w *h < 2500)
	quality = 90;

  if (quality != m_quality)
  {
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);
	if(quality >= 90) 
		cinfo.dct_method = JDCT_ISLOW;
	else 
		cinfo.dct_method = JDCT_FASTEST;
	if(quality >= 60) {
		cinfo.comp_info[0].h_samp_factor = 2;
		cinfo.comp_info[0].v_samp_factor = 2;
	}
	if(quality >= 70) {
		cinfo.comp_info[0].h_samp_factor = 1;
		cinfo.comp_info[0].v_samp_factor = 1;
	}
	m_quality = quality;
  }

  JpegSetDstManager(&cinfo, dst, dst_size);

  checkRowPointer(h);
  for (int dy = 0; dy < h; dy++)
    m_rowPointer[dy] = (JSAMPROW)(&srcBuf[dy * w * 4]);

  jpeg_start_compress(&cinfo, TRUE);
  while (cinfo.next_scanline < cinfo.image_height)
  {
    jpeg_write_scanlines(&cinfo, &m_rowPointer[cinfo.next_scanline],
      cinfo.image_height - cinfo.next_scanline);
	if (jpegError)
			break;
  }

  if (!jpegError)
		jpeg_finish_compress(&cinfo);

  if (jpegError) {
	jpeg_destroy_compress(&cinfo);
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	m_quality = 0;
	return 0;
  }
  return jpegDstDataLen;
}


void
JpegInitDestination(j_compress_ptr cinfo)
{
	jpegError = false;
	jpegDstManager.next_output_byte = jpegDstBuffer;
	jpegDstManager.free_in_buffer = jpegDstBufferLen;
}

boolean
JpegEmptyOutputBuffer(j_compress_ptr cinfo)
{
	jpegError = true;
	jpegDstManager.next_output_byte = jpegDstBuffer;
	jpegDstManager.free_in_buffer = jpegDstBufferLen;

	return TRUE;
}

void
JpegTermDestination(j_compress_ptr cinfo)
{
	jpegDstDataLen = jpegDstBufferLen - jpegDstManager.free_in_buffer;
}

void
JpegSetDstManager(j_compress_ptr cinfo, JOCTET *buf, size_t buflen)
{
	jpegDstBuffer = buf;
	jpegDstBufferLen = buflen;
	jpegDstManager.init_destination = JpegInitDestination;
	jpegDstManager.empty_output_buffer = JpegEmptyOutputBuffer;
	jpegDstManager.term_destination = JpegTermDestination;
	cinfo->dest = &jpegDstManager;
}
