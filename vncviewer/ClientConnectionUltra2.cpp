// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


// Zlib encoding
//
// The bits of the ClientConnection object to do with zlib.
#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"
#include "ViewerBounds.h"
#ifdef _VCPKG
#include <jpeglib.h>
#else
#include "libjpeg-turbo-win/jpeglib.h"
#endif
#include "Exception.h"
#include "ViewerJpegSource.h"
#include <memory>
#include <vector>



static void JpegInitSource(j_decompress_ptr cinfo);
static boolean JpegFillInputBuffer(j_decompress_ptr cinfo);
static void JpegSkipInputData(j_decompress_ptr cinfo, long num_bytes);
static void JpegTermSource(j_decompress_ptr cinfo);

static void
JpegInitSource(j_decompress_ptr cinfo)
{
  ViewerJpeg::initSource(cinfo);
}

static boolean
JpegFillInputBuffer(j_decompress_ptr cinfo)
{
  return ViewerJpeg::fillInputBuffer(cinfo);
}

static void
JpegSkipInputData(j_decompress_ptr cinfo, long num_bytes)
{
  ViewerJpeg::skipInputData(cinfo, num_bytes);
}

static void
JpegTermSource(j_decompress_ptr cinfo)
{
  /* No work necessary here. */
}

/*static void
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
}*/

void 
	ClientConnection::DecompressJpegRect(BYTE *src, int srclen, BYTE *dst, int dst_size,int w, int h,rfbPixelFormat m_remoteformat)
{
  if (srclen <= 0 || dst_size < 0 || w <= 0 || h <= 0 ||
      !ViewerBounds::pixels(w, h, m_remoteformat.bitsPerPixel / 8, dst_size))
      throw ErrorException(L"Invalid JPEG buffer dimensions.");
  struct jpeg_decompress_struct cinfo = {};
  auto destroy = [](jpeg_decompress_struct* p) { if (p->mem) jpeg_destroy_decompress(p); };
  std::unique_ptr<jpeg_decompress_struct, decltype(destroy)> cleanup(&cinfo, destroy);
  struct jpeg_error_mgr jerr;
  jpeg_source_mgr sourceManager = {};
  bool jpegError = false;
  JOCTET* jpegBufferPtr = (JOCTET*)src;
  size_t jpegBufferLen = srclen;
  cinfo.client_data = &jpegError;
  int pixelsize;
  BYTE *dstBuf=NULL;
  bool dstBufIsTemp = false;
  std::vector<BYTE> temporary;

 // Set up JPEG decompression
  cinfo.err = jpeg_std_error(&jerr);
  jerr.error_exit = JpegErrorHeader;
  try
  {
      jpeg_create_decompress(&cinfo);
      //JpegSetSrcManager(&cinfo, (char*)src, srclen);

      jpegBufferPtr = (JOCTET*)src;
      jpegBufferLen = (size_t)srclen;

      sourceManager.init_source = JpegInitSource;
      sourceManager.fill_input_buffer = JpegFillInputBuffer;
      sourceManager.skip_input_data = JpegSkipInputData;
      sourceManager.resync_to_restart = jpeg_resync_to_restart;
      sourceManager.term_source = JpegTermSource;
      sourceManager.next_input_byte = jpegBufferPtr;
      sourceManager.bytes_in_buffer = jpegBufferLen;

      cinfo.src = &sourceManager;

      jpeg_read_header(&cinfo, TRUE);
      cinfo.out_color_space = JCS_RGB;
      pixelsize = 3;
  }
  catch (QuietException&) {
      throw;
  }

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
    if (!ViewerBounds::pixels(w, h, pixelsize, 104857600))
      throw ErrorException(L"JPEG temporary buffer exceeds size limit.");
    temporary.resize(static_cast<size_t>(w) * h * pixelsize);
    dstBuf = temporary.data();
    dstBufIsTemp = true;
  }

  std::vector<JSAMPROW> rowPointer(h);
  for (int dy = 0; dy < h; dy++)
    rowPointer[dy] = (JSAMPROW)(&dstBuf[dy * w * pixelsize]);

  jpeg_start_decompress(&cinfo);
  if (cinfo.output_width != (unsigned)w || cinfo.output_height != (unsigned)h ||
      cinfo.output_components != pixelsize) {
      throw ErrorException(L"JPEG dimensions do not match update rectangle.");
  }

  while (cinfo.output_scanline < cinfo.output_height) {
    jpeg_read_scanlines(&cinfo, &rowPointer[cinfo.output_scanline],
			cinfo.output_height - cinfo.output_scanline);
    if (jpegError) {
      break;
    }
  }

  if (jpegError)
    throw ErrorException(L"Truncated JPEG rectangle.");
  jpeg_finish_decompress(&cinfo);
  if (dstBufIsTemp) {
    const int bytesPerPixel = m_remoteformat.bitsPerPixel / 8;
    if (m_remoteformat.redMax == 0 || m_remoteformat.greenMax == 0 || m_remoteformat.blueMax == 0 ||
        m_remoteformat.redShift >= 32 || m_remoteformat.greenShift >= 32 || m_remoteformat.blueShift >= 32)
      throw ErrorException(L"Invalid JPEG pixel format.");
    for (size_t i = 0; i < static_cast<size_t>(w) * h; ++i) {
      CARD32 pixel = ((static_cast<CARD32>(dstBuf[i * 3]) * m_remoteformat.redMax / 255) << m_remoteformat.redShift) |
        ((static_cast<CARD32>(dstBuf[i * 3 + 1]) * m_remoteformat.greenMax / 255) << m_remoteformat.greenShift) |
        ((static_cast<CARD32>(dstBuf[i * 3 + 2]) * m_remoteformat.blueMax / 255) << m_remoteformat.blueShift);
      memcpy(dst + i * bytesPerPixel, &pixel, bytesPerPixel);
    }
  }
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
	if (m_DIBbits) ConvertAll_secure(pfburh->r.w,pfburh->r.h,pfburh->r.x, pfburh->r.y,m_myFormat.bitsPerPixel/8,(BYTE *)m_zlibbuf,(BYTE *)m_DIBbits,m_si.framebufferWidth, numRawBytes, m_si.framebufferHeight);

}