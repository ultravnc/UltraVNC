/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
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
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


// Hextile Encoding
//
// The bits of the ClientConnection object to do with Hextile.

#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"

#include "lzo/minilzo.h"

bool ClientConnection::zlibDecompress(unsigned char *from_buf, unsigned char *to_buf, unsigned int count, unsigned int size, UltraVncZ * ultravncZ, bool zstd)
{
	int inflateResult;
	inflateResult = ultravncZ->decompress(count, size, from_buf, to_buf, zstd);
	if ( inflateResult < 0 ) {
		vnclog.Print(0, _T("zlib inflate error: %d\n"), inflateResult);
		return false;
	}

	return true;
}

void ClientConnection::ReadZlibHexRect(rfbFramebufferUpdateRectHeader *pfburh, bool zstd)
{
	switch (m_myFormat.bitsPerPixel) {
	case 8:
		HandleZlibHexEncoding8(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h, zstd);
		break;
	case 16:
		HandleZlibHexEncoding16(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h, zstd);
		break;
	case 32:
		HandleZlibHexEncoding32(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h, zstd);
		break;
	}
}

BYTE pfgcolor[4];
BYTE pbgcolor[4];

#define DEFINE_HEXTILE(bpp)														\
void ClientConnection::HandleZlibHexEncoding##bpp(int rx, int ry, int rw, int rh, bool zstd)		\
{																				\
    int x, y, w, h;																\
    CARD8 subencoding;															\
    CARD16 nCompData;															\
																				\
    CheckBufferSize((( 16 * 16 + 2 ) * bpp / 8 ) + 20 );											\
    CheckZlibBufferSize((( 16 * 16 + 2 ) * bpp / 8 ) + 20 );										\
																				\
    for (y = ry; y < ry+rh; y += 16) {											\
		omni_mutex_lock l(m_bitmapdcMutex);										\
		for (x = rx; x < rx+rw; x += 16) {										\
            w = h = 16;															\
            if (rx+rw - x < 16)													\
                w = rx+rw - x;													\
            if (ry+rh - y < 16)													\
                h = ry+rh - y;													\
																				\
            ReadExact((char *)&subencoding, 1);									\
																				\
            if (subencoding & rfbHextileRaw) {									\
				if (getBufferSize() < w * h * (bpp / 8) && w > 0 && h > 0) { 	\
					assert(true);												\
					return;														\
				}																\
                ReadExact(m_netbuf, w * h * (bpp / 8));							\
                SETPIXELS(m_netbuf, bpp, x,y,w,h)								\
                continue;														\
            }																	\
																				\
            if (subencoding & rfbHextileZlibRaw) {								\
                ReadExact((char *)&nCompData, 2);								\
                nCompData = Swap16IfLE(nCompData);								\
				CheckBufferSize(nCompData);										\
                ReadExact(m_netbuf, nCompData);									\
		        if (zlibDecompress((unsigned char *)m_netbuf, m_zlibbuf, nCompData, ((w*h+2)*(bpp/8)), ultraVncZRaw, zstd)) {  \
                    SETPIXELS(m_zlibbuf, bpp, x,y,w,h);							\
				}																\
                continue;														\
            }																	\
																				\
            if (subencoding & rfbHextileZlibHex) {								\
                ReadExact((char *)&nCompData, 2);								\
                nCompData = Swap16IfLE(nCompData);								\
				CheckBufferSize(nCompData);										\
                ReadExact(m_netbuf, nCompData);									\
		        if (zlibDecompress((unsigned char *)m_netbuf, m_zlibbuf, nCompData, ((w*h+2)*(bpp/8)+20), ultraVncZEncoded, zstd)) {  \
                    HandleZlibHexSubencodingBuf##bpp(x, y, w, h, subencoding, m_zlibbuf);							\
				}																\
                continue;														\
            }																	\
			else {																\
				HandleZlibHexSubencodingStream##bpp(x, y, w, h, subencoding);	\
			}																	\
																				\
        }																		\
    }																			\
																				\
}																				\
																				\
void ClientConnection::HandleZlibHexSubencodingStream##bpp(int x, int y, int w, int h, int subencoding)		\
{																				\
    CARD##bpp bg, fg;															\
    int i;																			\
    CARD8 *ptr;																	\
    int sx, sy, sw, sh;															\
    CARD8 nSubrects;															\
																				\
	if (subencoding & rfbHextileBackgroundSpecified) {							\
		ReadExact((char *)&bg, (bpp/8));										\
		memcpy(pbgcolor,&bg,bpp/8);												\
	}																			\
	FillSolidRect_ultra(x,y,w,h, m_myFormat.bitsPerPixel,(BYTE*)pbgcolor);\
																				\
	if (subencoding & rfbHextileForegroundSpecified)  {							\
		ReadExact((char *)&fg, (bpp/8));										\
		memcpy(pfgcolor,&fg,bpp/8);												\
	}																			\
																				\
	if (!(subencoding & rfbHextileAnySubrects)) {								\
		return;																	\
	}																			\
																				\
	ReadExact( (char *)&nSubrects, 1);											\
																				\
	ptr = (CARD8 *)m_netbuf;													\
																				\
	if (subencoding & rfbHextileSubrectsColoured) {								\
																				\
		ReadExact( m_netbuf, nSubrects * (2 + (bpp / 8)));						\
																				\
		for (i = 0; i < (int)nSubrects; i++) {								\
			memcpy(pfgcolor,ptr,bpp/8);											\
			ptr += (bpp/8);														\
			sx = *ptr >> 4;														\
			sy = *ptr++ & 0x0f;													\
			sw = (*ptr >> 4) + 1;												\
			sh = (*ptr++ & 0x0f) + 1;											\
			FillSolidRect_ultra(x+sx, y+sy, sw, sh, m_myFormat.bitsPerPixel,(BYTE*)pfgcolor);\
		}																		\
																				\
	} else {																	\
		ReadExact(m_netbuf, nSubrects * 2);										\
																				\
		for (i = 0; i < (int)nSubrects; i++) {								\
			sx = *ptr >> 4;														\
			sy = *ptr++ & 0x0f;													\
			sw = (*ptr >> 4) + 1;												\
			sh = (*ptr++ & 0x0f) + 1;											\
			FillSolidRect_ultra(x+sx, y+sy, sw, sh, m_myFormat.bitsPerPixel,(BYTE*)pfgcolor);\
		}																		\
	}																			\
}																				\
																				\
																				\
void ClientConnection::HandleZlibHexSubencodingBuf##bpp(int x, int y, int w, int h, int subencoding, unsigned char *buffer)		\
{																				\
	CARD##bpp bg, fg;															\
	int i;																			\
	CARD8 *ptr;																	\
	int sx, sy, sw, sh;															\
	CARD8 nSubrects;															\
	int bufIndex = 0;															\
																				\
	if (subencoding & rfbHextileBackgroundSpecified) {							\
		bg = *((CARD##bpp *)(buffer + bufIndex));								\
		bufIndex += (bpp/8);													\
		/* ReadExact((char *)&bg, (bpp/8)); */									\
		memcpy(pbgcolor,&bg,bpp/8);												\
	}																			\
	FillSolidRect_ultra(x,y,w,h, m_myFormat.bitsPerPixel,(BYTE*)pbgcolor);\
																				\
	if (subencoding & rfbHextileForegroundSpecified)  {							\
		fg = *((CARD##bpp *)(buffer + bufIndex));								\
		bufIndex += (bpp/8);													\
		/* ReadExact((char *)&fg, (bpp/8)); */									\
		memcpy(pfgcolor,&fg,bpp/8);												\
	}																			\
																				\
	if (!(subencoding & rfbHextileAnySubrects)) {								\
		return;																	\
	}																			\
																				\
	nSubrects = *((CARD8 *)(buffer + bufIndex));									\
	bufIndex += 1;																\
	/* ReadExact( (char *)&nSubrects, 1); */									\
																				\
	ptr = (CARD8 *)(buffer + bufIndex);											\
																				\
	if (subencoding & rfbHextileSubrectsColoured) {								\
																				\
		/* ReadExact( m_netbuf, nSubrects * (2 + (bpp / 8))); */				\
																				\
		for (i = 0; i < (int)nSubrects; i++) {								\
			memcpy(pfgcolor,ptr,bpp/8);											\
			ptr += (bpp/8);														\
			sx = *ptr >> 4;														\
			sy = *ptr++ & 0x0f;													\
			sw = (*ptr >> 4) + 1;												\
			sh = (*ptr++ & 0x0f) + 1;											\
			FillSolidRect_ultra(x+sx, y+sy, sw, sh, m_myFormat.bitsPerPixel,(BYTE*)pfgcolor);\
		}																		\
																				\
	} else {																	\
		/* ReadExact(m_netbuf, nSubrects * 2); */								\
																				\
		for (i = 0; i < (int)nSubrects; i++) {								\
			sx = *ptr >> 4;														\
			sy = *ptr++ & 0x0f;													\
			sw = (*ptr >> 4) + 1;												\
			sh = (*ptr++ & 0x0f) + 1;											\
			FillSolidRect_ultra(x+sx, y+sy, sw, sh, m_myFormat.bitsPerPixel,(BYTE*)pfgcolor);\
		}																		\
	}																			\
}


DEFINE_HEXTILE(8)
DEFINE_HEXTILE(16)
DEFINE_HEXTILE(32)


