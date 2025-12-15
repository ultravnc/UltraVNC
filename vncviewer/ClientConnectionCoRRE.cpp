// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


// CoRRE (Compact Rising Rectangle Encoding)
//
// The bits of the ClientConnection object to do with CoRRE.

#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"
#include "Exception.h"
extern char sz_L70[64];

void ClientConnection::ReadCoRRERect(rfbFramebufferUpdateRectHeader *pfburh)
{
	// An RRE rect is always followed by a background color
	// For speed's sake we read them together into a buffer.
	char tmpbuf[sz_rfbRREHeader+4];			// biggest pixel is 4 bytes long
    rfbRREHeader *prreh = (rfbRREHeader *) tmpbuf;
	CARD8 *pcolor = (CARD8 *) tmpbuf + sz_rfbRREHeader;
	ReadExact(tmpbuf, sz_rfbRREHeader + m_minPixelBytes);

	prreh->nSubrects = Swap32IfLE(prreh->nSubrects);

    // Draw the background of the rectangle
	FillSolidRect_ultra(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h, m_myFormat.bitsPerPixel,pcolor);

    if (prreh->nSubrects == 0) return;
	if (prreh->nSubrects > 20000000) 
		throw ErrorException(sz_L70);

	// Draw the sub-rectangles
    rfbCoRRERectangle *pRect;
	rfbRectangle rect;

	// The size of an CoRRE subrect including color info
	int subRectSize = m_minPixelBytes + sz_rfbCoRRERectangle;

	// Read subrects into the buffer 
	CheckBufferSize(subRectSize * prreh->nSubrects);
    ReadExact(m_netbuf, subRectSize * prreh->nSubrects);
	BYTE *p = (BYTE *) m_netbuf;

	{
		omni_mutex_lock l(m_bitmapdcMutex);

		for (CARD32 i = 0; i < prreh->nSubrects; i++) {
			
			pRect = (rfbCoRRERectangle *) (p + m_minPixelBytes);
			
			rect.x = pRect->x + pfburh->r.x;
			rect.y = pRect->y + pfburh->r.y;
			rect.w = pRect->w;
			rect.h = pRect->h;
			FillSolidRect_ultra(rect.x, rect.y, rect.w, rect.h, m_myFormat.bitsPerPixel,p);
			p+=subRectSize;
		}
	}

}