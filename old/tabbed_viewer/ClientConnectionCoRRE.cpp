//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
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
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// CoRRE (Compact Rising Rectangle Encoding)
//
// The bits of the ClientConnection object to do with CoRRE.

#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"


void ClientConnection::ReadCoRRERect(rfbFramebufferUpdateRectHeader *pfburh)
{
	// An RRE rect is always followed by a background color
	// For speed's sake we read them together into a buffer.
	char tmpbuf[sz_rfbRREHeader+4];			// biggest pixel is 4 bytes long
    rfbRREHeader *prreh = (rfbRREHeader *) tmpbuf;
	CARD8 *pcolor = (CARD8 *) tmpbuf + sz_rfbRREHeader;
	ReadExact(tmpbuf, sz_rfbRREHeader + m_minPixelBytes);

	prreh->nSubrects = Swap32IfLE(prreh->nSubrects);

	SETUP_COLOR_SHORTCUTS;
	omni_mutex_lock l(m_bitmapdcMutex);	

    switch (m_myFormat.bitsPerPixel) {
        case 8:
			FillSolidRect8(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h, (CARD8*)pcolor);
			break;
        case 16:
			FillSolidRect16(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h, (CARD16*)pcolor);
			break;
        case 24:
        case 32:
			FillSolidRect32(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h, (CARD32*)pcolor);
			break;
    }

    if (prreh->nSubrects == 0) return;

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
		for (CARD32 i = 0; i < prreh->nSubrects; i++) {
			
			pRect = (rfbCoRRERectangle *) (p + m_minPixelBytes);
			rect.x = pRect->x + pfburh->r.x;
			rect.y = pRect->y + pfburh->r.y;
			rect.w = pRect->w;
			rect.h = pRect->h;
			
			switch (m_myFormat.bitsPerPixel) {
			case 8:
				FillSolidRect8(rect.x, rect.y, rect.w, rect.h, (CARD8*)p);
				break;
			case 16:
				FillSolidRect16(rect.x, rect.y, rect.w, rect.h, (CARD16*)p);
				break;
			case 32:
				FillSolidRect32(rect.x, rect.y, rect.w, rect.h, (CARD32*)p);
				break;
			};
			p+=subRectSize;
		}
	}

}