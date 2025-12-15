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


// CopyRect Encoding
//
// The bits of the ClientConnection object to do with CopyRect.

#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"

void ClientConnection::ReadCopyRect(rfbFramebufferUpdateRectHeader *pfburh) {
	rfbCopyRect cr;
	ReadExact((char *) &cr, sz_rfbCopyRect);
	cr.srcX = Swap16IfLE(cr.srcX); 
	cr.srcY = Swap16IfLE(cr.srcY);
	// By sure the rect is insite the border memcopy does not like it 
	if (!Check_Rectangle_borders(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h)) return;
	if (!Check_Rectangle_borders(cr.srcX, cr.srcY, pfburh->r.w, pfburh->r.h)) return;

	// tight If *Cursor encoding is used, we should extend our "cursor lock area"
	// (previously set to destination rectangle) to the source rect as well.
	SoftCursorLockArea(cr.srcX, cr.srcY, pfburh->r.w, pfburh->r.h);

	if (m_DIBbits)
		{
		omni_mutex_lock l(m_bitmapdcMutex);
		int bytesPerInputRow = m_si.framebufferWidth * m_myFormat.bitsPerPixel/8;

		// Pitch
		if (bytesPerInputRow % 4)
		bytesPerInputRow += 4 - bytesPerInputRow % 4;

		int bytesPerOutputRow = pfburh->r.w * m_myFormat.bitsPerPixel/8;
		int OutputHeight=pfburh->r.h;
		BYTE *sourcepos,*iptr,*destpos,*optr;
		if (cr.srcY<=pfburh->r.y)
		{
			{
				destpos = (BYTE*)m_DIBbits + (bytesPerInputRow * (pfburh->r.y+pfburh->r.h-1))+(pfburh->r.x * m_myFormat.bitsPerPixel/8);
			sourcepos = (BYTE*)m_DIBbits + (bytesPerInputRow * (cr.srcY+pfburh->r.h-1))+((cr.srcX) * m_myFormat.bitsPerPixel/8);
				iptr=sourcepos;
				optr=destpos;
				while (OutputHeight > 0) {
					memcpy(optr, iptr, bytesPerOutputRow);			
					iptr -= bytesPerInputRow;
					optr -= bytesPerInputRow;
					OutputHeight--;
				}
			}
		}
		else if (cr.srcY>pfburh->r.y)
		{
			{
				destpos = (BYTE*)m_DIBbits + (bytesPerInputRow * pfburh->r.y)+(pfburh->r.x * m_myFormat.bitsPerPixel/8);
				sourcepos = (BYTE*)m_DIBbits + (bytesPerInputRow * (cr.srcY))+((cr.srcX) * m_myFormat.bitsPerPixel/8);
				iptr=sourcepos;
				optr=destpos;
				while (OutputHeight > 0) {
					memcpy(optr, iptr, bytesPerOutputRow);				
					iptr += bytesPerInputRow;
					optr += bytesPerInputRow;
					OutputHeight--;
				}
			}
		}
	}
}
