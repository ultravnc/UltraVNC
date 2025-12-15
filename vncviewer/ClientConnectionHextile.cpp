// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"

void ClientConnection::ReadHextileRect(rfbFramebufferUpdateRectHeader *pfburh)
{
	switch (m_myFormat.bitsPerPixel) {
	case 8:
		HandleHextileEncoding8(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h);
		break;
	case 16:
		HandleHextileEncoding16(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h);
		break;
	case 32:
		HandleHextileEncoding32(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h);
		break;
	}
}


#define DEFINE_HEXTILE(bpp)                                                   \
void ClientConnection::HandleHextileEncoding##bpp(int rx, int ry, int rw, int rh)                    \
{                                                                             \
    CARD##bpp bg, fg;                                                         \
    int i;                                                                    \
    CARD8 *ptr;                                                               \
    int x, y, w, h;                                                           \
    int sx, sy, sw, sh;                                                       \
    CARD8 subencoding;                                                        \
    CARD8 nSubrects;                                                          \
																			  \
    CheckBufferSize( 16 * 16 * bpp );										  \
                                                                              \
    for (y = ry; y < ry+rh; y += 16) {                                        \
		omni_mutex_lock l(m_bitmapdcMutex);									  \
		for (x = rx; x < rx+rw; x += 16) {                                    \
            w = h = 16;                                                       \
            if (rx+rw - x < 16)                                               \
                w = rx+rw - x;                                                \
            if (ry+rh - y < 16)                                               \
                h = ry+rh - y;                                                \
                                                                              \
            ReadExact((char *)&subencoding, 1);                               \
                                                                              \
            if (subencoding & rfbHextileRaw) {                                \
				if (getBufferSize() < w * h * (bpp / 8) && w > 0 && h > 0){  	\
					assert(true);											\
					return;													\
				}															\
                ReadExact(m_netbuf, w * h * (bpp / 8));                       \
                SETPIXELS(m_netbuf, bpp, x,y,w,h)							 \
                continue;                                                     \
            }                                                                 \
                                                                              \
		    if (subencoding & rfbHextileBackgroundSpecified) {                \
                ReadExact((char *)&bg, (bpp/8));                              \
			}																  \
            FillSolidRect_ultra(x,y,w,h, m_myFormat.bitsPerPixel,(BYTE*)&bg);\
                                                                              \
            if (subencoding & rfbHextileForegroundSpecified)  {               \
                ReadExact((char *)&fg, (bpp/8));                              \
			}                                                                 \
                                                                              \
            if (!(subencoding & rfbHextileAnySubrects)) {                     \
                continue;                                                     \
            }                                                                 \
                                                                              \
            ReadExact( (char *)&nSubrects, 1) ;                               \
                                                                              \
            ptr = (CARD8 *)m_netbuf;                                          \
                                                                              \
            if (subencoding & rfbHextileSubrectsColoured) {                   \
				                                                              \
				if (getBufferSize() < nSubrects * (2 + (bpp / 8)) && nSubrects > 0) {  \
					assert(true);											 \
					return;													  \
				}															\
                ReadExact( m_netbuf, nSubrects * (2 + (bpp / 8)));            \
                                                                              \
                for (i = 0; i < (int)nSubrects; i++) {                        \
					memcpy(&fg,ptr,bpp/8);									  \
					ptr += (bpp/8);                                           \
                    sx = *ptr >> 4;                                           \
                    sy = *ptr++ & 0x0f;                                       \
                    sw = (*ptr >> 4) + 1;                                     \
                    sh = (*ptr++ & 0x0f) + 1;                                 \
                    FillSolidRect_ultra(x+sx, y+sy, sw, sh, m_myFormat.bitsPerPixel,(BYTE*)&fg);\
                }                                                             \
                                                                              \
            } else {                                                          \
			    if (getBufferSize() < (nSubrects * 2) && nSubrects > 0) {      \
					assert(true);											\
					return;													  \
				}															\
                ReadExact(m_netbuf, nSubrects * 2);                           \
                                                                              \
                for (i = 0; i < (int)nSubrects; i++) {                        \
                    sx = *ptr >> 4;                                           \
                    sy = *ptr++ & 0x0f;                                       \
                    sw = (*ptr >> 4) + 1;                                     \
                    sh = (*ptr++ & 0x0f) + 1;                                 \
                    FillSolidRect_ultra(x+sx, y+sy, sw, sh, m_myFormat.bitsPerPixel,(BYTE*)&fg);\
                }                                                             \
            }																\
			/*Sleep(0);*/														  \
        }                                                                     \
    }                                                                         \
                                                                              \
}

DEFINE_HEXTILE(8)
DEFINE_HEXTILE(16)
DEFINE_HEXTILE(32)


