// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


// RRE (Rising Rectangle Encoding)
//
// The bits of the ClientConnection object to do with RRE.

#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"
#include "Exception.h"
extern char sz_L70[64];

void ClientConnection::ReadRRERect(rfbFramebufferUpdateRectHeader *pfburh)
{
	// An RRE rect is always followed by a background color
	// For speed's sake we read them together into a buffer.
	char tmpbuf[sz_rfbRREHeader+4];			// biggest pixel is 4 bytes long
    rfbRREHeader *prreh = (rfbRREHeader *) tmpbuf;
	CARD8 *pcolor = (CARD8 *) tmpbuf + sz_rfbRREHeader;
	ReadExact(tmpbuf, sz_rfbRREHeader + m_minPixelBytes);

	prreh->nSubrects = Swap32IfLE(prreh->nSubrects);
	
	// No other threads can use bitmap DC
	omni_mutex_lock l(m_bitmapdcMutex);
		
	// Draw the background of the rectangle
	FillSolidRect_ultra(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h, m_myFormat.bitsPerPixel,pcolor);
	
    if (prreh->nSubrects == 0) return;
	if (prreh->nSubrects > 20000000) 
		throw ErrorException(sz_L70);
	
	// Draw the sub-rectangles
    rfbRectangle rect, *pRect;
	// The size of an RRE subrect including color info
	int subRectSize = m_minPixelBytes + sz_rfbRectangle;
    
	// Read subrects into the buffer 
	CheckBufferSize(subRectSize * prreh->nSubrects);
    ReadExact(m_netbuf, subRectSize * prreh->nSubrects);
	BYTE *p = (BYTE *) m_netbuf;
	for (CARD32 i = 0; i < prreh->nSubrects; i++) {
		pRect = (rfbRectangle *) (p + m_minPixelBytes);
		
		rect.x = (CARD16) (Swap16IfLE(pRect->x) + pfburh->r.x);
		rect.y = (CARD16) (Swap16IfLE(pRect->y) + pfburh->r.y);
		rect.w = Swap16IfLE(pRect->w);
		rect.h = Swap16IfLE(pRect->h);
		
		FillSolidRect_ultra(rect.x, rect.y, rect.w, rect.h, m_myFormat.bitsPerPixel,p);
		p+=subRectSize;
	}
}
