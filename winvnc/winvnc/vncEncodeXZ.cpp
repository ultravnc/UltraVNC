// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//


#ifdef _XZ
#include "stdhdrs.h"
#include "vncEncodeXZ.h"
#include "rfb.h"
#include "rfbMisc.h"
#include <stdlib.h>
#include <time.h>
#include <rdr/MemOutStream.h>
#include <rdr/xzOutStream.h>

#define GET_IMAGE_INTO_BUF(tx,ty,tw,th,buf)     \
  rfb::Rect rect;                                    \
  rect.tl.x = tx;                               \
  rect.tl.y = ty;                                \
  rect.br.x = tx+tw;                           \
  rect.br.y = ty+th;                          \
  encoder->Translate(source, (BYTE*)buf, rect);

#define EXTRA_ARGS , BYTE* source, vncEncoder* encoder

#define ENDIAN_LITTLE 0
#define ENDIAN_BIG 1
#define ENDIAN_NO 2
#define BPP 8
#define XZYW_ENDIAN ENDIAN_NO
#include <rfb/xzEncode.h>
#undef BPP
#define BPP 15
#undef XZYW_ENDIAN
#define XZYW_ENDIAN ENDIAN_LITTLE
#include <rfb/xzEncode.h>
#undef XZYW_ENDIAN
#define XZYW_ENDIAN ENDIAN_BIG
#include <rfb/xzEncode.h>
#undef BPP
#define BPP 16
#undef XZYW_ENDIAN
#define XZYW_ENDIAN ENDIAN_LITTLE
#include <rfb/xzEncode.h>
#undef XZYW_ENDIAN
#define XZYW_ENDIAN ENDIAN_BIG
#include <rfb/xzEncode.h>
#undef BPP
#define BPP 32
#undef XZYW_ENDIAN
#define XZYW_ENDIAN ENDIAN_LITTLE
#include <rfb/xzEncode.h>
#undef XZYW_ENDIAN
#define XZYW_ENDIAN ENDIAN_BIG
#include <rfb/xzEncode.h>
#define CPIXEL 24A
#undef XZYW_ENDIAN
#define XZYW_ENDIAN ENDIAN_LITTLE
#include <rfb/xzEncode.h>
#undef XZYW_ENDIAN
#define XZYW_ENDIAN ENDIAN_BIG
#include <rfb/xzEncode.h>
#undef CPIXEL
#define CPIXEL 24B
#undef XZYW_ENDIAN
#define XZYW_ENDIAN ENDIAN_LITTLE
#include <rfb/xzEncode.h>
#undef XZYW_ENDIAN
#define XZYW_ENDIAN ENDIAN_BIG
#include <rfb/xzEncode.h>
#undef CPIXEL
#undef BPP

vncEncodeXZ::vncEncodeXZ()
{
	mos = new rdr::MemOutStream;
	xzos = new rdr::xzOutStream;
	beforeBuf = new rdr::U32[rfbXZTileWidth * rfbXZTileHeight + 1];
	m_use_xzyw = FALSE;
}

vncEncodeXZ::~vncEncodeXZ()
{
	delete mos;
	delete xzos;
	delete [] beforeBuf;
}

void vncEncodeXZ::Init()
{
	vncEncoder::Init();
}

UINT vncEncodeXZ::RequiredBuffSize(UINT width, UINT height)
{
	// this is a guess - 128 bytes plus 2 times raw
	return (sz_rfbFramebufferUpdateRectHeader + sz_rfbXZHeader + 128 +
		width * height * (m_remoteformat.bitsPerPixel / 8) * 2);
}

UINT vncEncodeXZ::EncodeBulkRects(const rfb::RectVector &allRects, BYTE *source, BYTE *dest, VSocket *outConn)
{
	if (allRects.empty()) {
		return TRUE;
	}

	xzos->SetCompressLevel(m_compresslevel);
	mos->clear();
	xzos->setUnderlying(mos);
	
	int nAllRects = (int)allRects.size();

	rfb::RectVector::const_iterator i;

	for (i=allRects.begin();i!=allRects.end();i++) {
		const rfb::Rect& rect(*i);

		rfbRectangle rectangle;
		rectangle.x = Swap16IfLE(rect.tl.x- monitor_Offsetx);
		rectangle.y = Swap16IfLE(rect.tl.y- monitor_Offsety);
		rectangle.w = Swap16IfLE(rect.br.x - rect.tl.x);
		rectangle.h = Swap16IfLE(rect.br.y - rect.tl.y);

		xzos->writeBytes((const void*)&rectangle, sz_rfbRectangle);
	}
	
	for (i=allRects.begin();i!=allRects.end();i++) {
		const rfb::Rect& rect(*i);

		int x = rect.tl.x;
		int y = rect.tl.y;
		int w = rect.br.x - x;
		int h = rect.br.y - y;

		EncodeRect_Internal(source, x, y, w, h);
	}

	xzos->flush();

	const void* pDataBytes = mos->data();
	int nDataLength = mos->length();
	
	assert(nDataLength > 0);

	rfbFramebufferUpdateRectHeader surh;
	if( m_use_xzyw ){
		surh.encoding = Swap32IfLE(rfbEncodingXZYW);
	}else{
		surh.encoding = Swap32IfLE(rfbEncodingXZ);
	}

	int nAllRectsHigh = (nAllRects & 0xFFFF0000) >> 16;
	int nAllRectsLow = (nAllRects & 0x0000FFFF);

	int nDataLengthHigh = (nDataLength & 0xFFFF0000) >> 16;
	int nDataLengthLow = (nDataLength & 0x0000FFFF);

	surh.r.x = Swap16IfLE(nAllRectsHigh);
	surh.r.y = Swap16IfLE(nAllRectsLow);
	surh.r.w = Swap16IfLE(nDataLengthHigh);
	surh.r.h = Swap16IfLE(nDataLengthLow);

	UINT nTotalLength = 0;
	if (!outConn->SendExact((const char*)&surh, sz_rfbFramebufferUpdateRectHeader)) {
		return FALSE;
	}
	nTotalLength += sz_rfbFramebufferUpdateRectHeader;

	if (!outConn->SendExact((const char*)pDataBytes, nDataLength)) {
		return FALSE;
	}
	nTotalLength += nDataLength;

	return nDataLength > 0 ? TRUE : FALSE;
}

UINT vncEncodeXZ::EncodeRect(BYTE *source, BYTE *dest, const rfb::Rect &rect)
{
	int x = rect.tl.x;
	int y = rect.tl.y;
	int w = rect.br.x - x;
	int h = rect.br.y - y;

	xzos->SetCompressLevel(m_compresslevel);
	mos->clear();
	xzos->setUnderlying(mos);

	EncodeRect_Internal(source, x, y, w, h);

	xzos->flush();

	rfbFramebufferUpdateRectHeader* surh = (rfbFramebufferUpdateRectHeader*)dest;
	surh->r.x = Swap16IfLE(x- monitor_Offsetx);
	surh->r.y = Swap16IfLE(y- monitor_Offsety);
	surh->r.w = Swap16IfLE(w);
	surh->r.h = Swap16IfLE(h);
	if( m_use_xzyw ){
		surh->encoding = Swap32IfLE(rfbEncodingXZYW);
	}else{
		surh->encoding = Swap32IfLE(rfbEncodingXZ);
	}

	rfbXZHeader* hdr = (rfbXZHeader*)(dest +
		sz_rfbFramebufferUpdateRectHeader);

	int length = mos->length();
	hdr->length = Swap32IfLE(length);

	memcpy(dest + sz_rfbFramebufferUpdateRectHeader + sz_rfbXZHeader,
		(rdr::U8*)mos->data(), length);

	return sz_rfbFramebufferUpdateRectHeader + sz_rfbXZHeader + length;
}

void vncEncodeXZ::EncodeRect_Internal(BYTE *source, int x, int y, int w, int h)
{
	if( m_use_xzyw ){
		if( m_qualitylevel < 0 ){
			xzyw_level = 1;
		}else if( m_qualitylevel < 3 ){
			xzyw_level = 3;
		}else if( m_qualitylevel < 6 ){
			xzyw_level = 2;
		}else{
			xzyw_level = 1;
		}
	}else{
		xzyw_level = 0;
	}

	switch (m_remoteformat.bitsPerPixel) {

  case 8:
	  xzEncode8NE( x, y, w, h, mos, xzos, beforeBuf, source, this);
	  break;

  case 16:
	  if( m_remoteformat.greenMax > 0x1F ){
		  if( m_remoteformat.bigEndian ){
			  xzEncode16BE(x, y, w, h, mos, xzos, beforeBuf, source, this);
		  }else{
			  xzEncode16LE(x, y, w, h, mos, xzos, beforeBuf, source, this);
		  }
	  }else{
		  if( m_remoteformat.bigEndian ){
			  xzEncode15BE(x, y, w, h, mos, xzos, beforeBuf, source, this);
		  }else{
			  xzEncode15LE(x, y, w, h, mos, xzos, beforeBuf, source, this);
		  }
	  }
	  break;

  case 32:
	  bool fitsInLS3Bytes
		  = ((m_remoteformat.redMax   << m_remoteformat.redShift)   < (1<<24) &&
		  (m_remoteformat.greenMax << m_remoteformat.greenShift) < (1<<24) &&
		  (m_remoteformat.blueMax  << m_remoteformat.blueShift)  < (1<<24));

	  bool fitsInMS3Bytes = (m_remoteformat.redShift   > 7  &&
		  m_remoteformat.greenShift > 7  &&
		  m_remoteformat.blueShift  > 7);

	  if ((fitsInLS3Bytes && !m_remoteformat.bigEndian) ||
		  (fitsInMS3Bytes && m_remoteformat.bigEndian))
	  {
		  if( m_remoteformat.bigEndian ){
			  xzEncode24ABE(x, y, w, h, mos, xzos, beforeBuf, source, this);
		  }else{
			  xzEncode24ALE(x, y, w, h, mos, xzos, beforeBuf, source, this);
		  }
	  }
	  else if ((fitsInLS3Bytes && m_remoteformat.bigEndian) ||
		  (fitsInMS3Bytes && !m_remoteformat.bigEndian))
	  {
		  if( m_remoteformat.bigEndian ){
			  xzEncode24BBE(x, y, w, h, mos, xzos, beforeBuf, source, this);
		  }else{
			  xzEncode24BLE(x, y, w, h, mos, xzos, beforeBuf, source, this);
		  }
	  }
	  else
	  {
		  if( m_remoteformat.bigEndian ){
			  xzEncode32BE(x, y, w, h, mos, xzos, beforeBuf, source, this);
		  }else{
			  xzEncode32LE(x, y, w, h, mos, xzos, beforeBuf, source, this);
		  }
	  }
	  break;
	}
}


#endif