/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
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
// http://ultravnc.sourceforge.net/
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
	if (m_bitsCache==NULL)
	{
		m_bitsCache= new BYTE[m_si.framebufferWidth*m_si.framebufferHeight* m_myFormat.bitsPerPixel / 8];
		//vnclog.Print(4, _T("m_bitsCache %i prt %d \n"),m_si.framebufferWidth*m_si.framebufferHeight* m_myFormat.bitsPerPixel / 8,m_bitsCache);
		//Sleep(2000);
	}


	int x = r.left;
	int y = r.top;
	int w = r.right - r.left;
	int h = r.bottom - r.top;
	omni_mutex_lock l(m_bitmapdcMutex);
	ConvertAll_source_dest(w, h, x, y,m_myFormat.bitsPerPixel / 8,(BYTE *)m_DIBbits,(BYTE *)m_bitsCache,m_si.framebufferWidth);
	}

void
ClientConnection::Check_m_bitsCacheSwapBuffer(int bufsize)
{
	if (m_bitsCacheSwapBufferSize > bufsize) return;
	BYTE *newbuf = new BYTE[bufsize+256];
	if (newbuf == NULL) {
		exit(0);
	}

	if (m_bitsCacheSwapBuffer != NULL)
		delete [] m_bitsCacheSwapBuffer;
	m_bitsCacheSwapBuffer = newbuf;
	m_bitsCacheSwapBufferSize=bufsize + 256;

}

void ClientConnection::RestoreArea(RECT &r)
{
	int x = r.left;
	int y = r.top;
	int w = r.right - r.left;
	int h = r.bottom - r.top;

	omni_mutex_lock l(m_bitmapdcMutex);
	Check_m_bitsCacheSwapBuffer(w*h*m_myFormat.bitsPerPixel / 8);
	ConvertAll_dest0_source(w, h, x, y,m_myFormat.bitsPerPixel / 8,m_bitsCacheSwapBuffer,(BYTE *)m_DIBbits,m_si.framebufferWidth);
	ConvertAll_source_dest(w, h, x, y,m_myFormat.bitsPerPixel / 8,m_bitsCache,(BYTE *)m_DIBbits,m_si.framebufferWidth);
	/*source0_dest*/ConvertAll(w, h, x, y,m_myFormat.bitsPerPixel / 8,m_bitsCacheSwapBuffer,(BYTE *)m_bitsCache,m_si.framebufferWidth);
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
	for (int i = 0 ; i < nNbCacheRects; i++)
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
		InvalidateRegion(&cacherect,prgn);
	}

}

