// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


//
// CACHERect Encoding
//
// .

#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"


void ClientConnection::ReadCacheRect(rfbFramebufferUpdateRectHeader *pfburh)
{
	rfbCacheRect cr;
	ReadExact((char *) &cr, sz_rfbCacheRect);
	cr.special = Swap16IfLE(cr.special); 
	
	RECT rect;
	rect.left=pfburh->r.x;
	rect.right=pfburh->r.x+pfburh->r.w;
	rect.top=pfburh->r.y;
	rect.bottom=pfburh->r.y+pfburh->r.h;
	RestoreArea(rect);
}

void ClientConnection::SaveArea(RECT &r)
{
	if (!m_opts->m_fEnableCache) return; // sf@2002

	int x = r.left;
	int y = r.top;
	int w = r.right - r.left;
	int h = r.bottom - r.top;

	omni_mutex_lock l(m_bitmapdcMutex);
	if (m_DIBbitsCache && m_DIBbits) 
		Copybuffer(w, h, x, y,m_myFormat.bitsPerPixel/8,(BYTE*)m_DIBbits,(BYTE*)m_DIBbitsCache,m_si.framebufferWidth,m_si.framebufferHeight);
}

void ClientConnection::RestoreArea(RECT &r)
{
	int x = r.left;
	int y = r.top;
	int w = r.right - r.left;
	int h = r.bottom - r.top;


	omni_mutex_lock l(m_bitmapdcMutex);

	if (m_DIBbitsCache && m_DIBbits)
		Switchbuffer(w, h, x, y,m_myFormat.bitsPerPixel/8,(BYTE*)m_DIBbits,(BYTE*)m_DIBbitsCache,m_si.framebufferWidth);
}

//
// sf@2002
// 
void ClientConnection::ClearCache()
{
	if (!m_opts->m_fEnableCache) return;
	if (m_DIBbitsCache) 
		memset(m_DIBbitsCache,0,m_si.framebufferWidth * m_si.framebufferHeight *m_myFormat.bitsPerPixel/8);
}


//
// sf@2002 
// - Read a cache rects zipped block coming from the server
// - Restore all these cache rects on the screen
void ClientConnection::ReadCacheZip(rfbFramebufferUpdateRectHeader *pfburh,HRGN *prgn)
{
	UINT nNbCacheRects = pfburh->r.x;

	UINT numRawBytes = nNbCacheRects * sz_rfbRectangle;
	numRawBytes += (numRawBytes/100) + 8;
	UINT numCompBytes;

	rfbZlibHeader hdr;
	// Read in the rfbZlibHeader
	ReadExact((char *)&hdr, sz_rfbZlibHeader);
	numCompBytes = Swap32IfLE(hdr.nBytes);

	// Check the net buffer
	CheckBufferSize(numCompBytes);

	// Read the compressed data
	ReadExact((char *)m_netbuf, numCompBytes);

	// Verify buffer space for cache rects list
	CheckZipBufferSize(numRawBytes);

	int nRet = uncompress((unsigned char*)m_zipbuf,// Dest  
						  (unsigned long*)&numRawBytes,// Dest len
						  (unsigned char*)m_netbuf,	// Src
						  numCompBytes	// Src len
						 );							    
	if (nRet != 0)
	{
		return;		
	}

	// Read all the cache rects
	rfbRectangle theRect;
	
	BYTE* p = m_zipbuf;
	for (UINT i = 0 ; i < nNbCacheRects; i++)
	{
		memcpy((BYTE*)&theRect, p, sz_rfbRectangle);
		p += sz_rfbRectangle;

		RECT cacherect;
		cacherect.left = Swap16IfLE(theRect.x);
		cacherect.right = Swap16IfLE(theRect.x) + Swap16IfLE(theRect.w);
		cacherect.top = Swap16IfLE(theRect.y);
		cacherect.bottom = Swap16IfLE(theRect.y) + Swap16IfLE(theRect.h);

		SoftCursorLockArea(cacherect.left, cacherect.top, cacherect.right - cacherect.left, cacherect.bottom - cacherect.top);
		RestoreArea(cacherect);
		if (!m_opts->m_Directx) InvalidateRegion(&cacherect,prgn);
	}

}

