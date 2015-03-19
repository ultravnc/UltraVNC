/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 UltraVNC Team Members. All Rights Reserved.
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
// http://www.uvnc.com
//
////////////////////////////////////////////////////////////////////////////
//
// CACHERect Encoding
//
// .

#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"
// #include "zlib\zlib.h"


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
	if (!m_opts.m_fEnableCache) return; // sf@2002

	int x = r.left;
	int y = r.top;
	int w = r.right - r.left;
	int h = r.bottom - r.top;

	omni_mutex_lock l(m_bitmapdcMutex);
	if (m_DIBbitsCache && m_DIBbits) 
		Copybuffer(w, h, x, y,m_myFormat.bitsPerPixel/8,(BYTE*)m_DIBbits,(BYTE*)m_DIBbitsCache,m_si.framebufferWidth);
}

void ClientConnection::RestoreArea(RECT &r)
{
	int x = r.left;
	int y = r.top;
	int w = r.right - r.left;
	int h = r.bottom - r.top;
	HBITMAP m_hTempBitmap=NULL;
	HDC		m_hTempBitmapDC=NULL;

	omni_mutex_lock l(m_bitmapdcMutex);

	if (m_DIBbitsCache && m_DIBbits)
		Switchbuffer(w, h, x, y,m_myFormat.bitsPerPixel/8,(BYTE*)m_DIBbits,(BYTE*)m_DIBbitsCache,m_si.framebufferWidth);
}

//
// sf@2002
// 
void ClientConnection::ClearCache()
{
	if (!m_opts.m_fEnableCache) return;
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
		if (!m_opts.m_Directx) InvalidateRegion(&cacherect,prgn);
	}

}

