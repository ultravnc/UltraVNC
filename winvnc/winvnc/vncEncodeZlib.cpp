/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2020 UltraVNC Team Members. All Rights Reserved.
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
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////

#include "stdhdrs.h"
#include "vncEncodeZlib.h"
#include "../../common/UltraVncZ.h"
//------------------------------------------------------------------
vncEncodeZlib::vncEncodeZlib()
{
	m_queueEnable = false;
	must_be_zipped = false;
	ultraVncZ = new UltraVncZ();
	m_buffer = NULL;
	m_Queuebuffer = NULL;
	m_QueueCompressedbuffer = NULL;
	m_bufflen = 0;
	m_Queuelen = 0;
	MaxQueuebufflen=128*1024;
	m_Queuebuffer = new BYTE [MaxQueuebufflen+1];
	if (m_Queuebuffer == NULL)
		vnclog.Print(LL_INTINFO, VNCLOG("Memory error"));
	m_QueueCompressedbuffer = new BYTE [MaxQueuebufflen+(MaxQueuebufflen/100)+8];
	if (m_Queuebuffer == NULL)
		vnclog.Print(LL_INTINFO, VNCLOG("Memory error"));
}
//------------------------------------------------------------------
vncEncodeZlib::~vncEncodeZlib()
{
	if (m_buffer != NULL) {
		delete [] m_buffer;
		m_buffer = NULL;
	}
	if (m_Queuebuffer != NULL) {
		delete [] m_Queuebuffer;
		m_Queuebuffer = NULL;
	}
	if (m_QueueCompressedbuffer != NULL)
	{
		delete [] m_QueueCompressedbuffer;
		m_QueueCompressedbuffer = NULL;
	}
	vnclog.Print(LL_INTINFO, VNCLOG("Zlib Xor encoder stats: rawdata=%d  protocol=%d compressed=%d transmitted=%d\n"),dataSize, rectangleOverhead, encodedSize,transmittedSize);
	if (dataSize != 0) {
		vnclog.Print(LL_INTINFO, VNCLOG("Zlib Xor encoder efficiency: %.3f%%\n"),(double)((double)((dataSize - transmittedSize) * 100) / dataSize));
	}
	delete ultraVncZ;
}
//------------------------------------------------------------------
void vncEncodeZlib::Init()
{
	totalraw=0;
	encodedSize=0;
	rectangleOverhead=0;
	transmittedSize=0;
	dataSize=0;
	vncEncoder::Init();
	m_nNbRects=0;
}
//------------------------------------------------------------------
UINT vncEncodeZlib::RequiredBuffSize(UINT width, UINT height)
{
	int result;

	// The zlib library specifies a maximum compressed size of
	// the raw size plus one percent plus 8 bytes.  We also need
	// to cover the zlib header space.
	result = vncEncoder::RequiredBuffSize(width, height);
	Firstrun=result*2;//Needed to exclude xor when cachebuffer is empty
	result += ((result / 100) + 8) + sz_rfbZlibHeader;
	return result;
}
//------------------------------------------------------------------
UINT vncEncodeZlib::NumCodedRects(const rfb::Rect &rect)
{
	const int rectW = rect.br.x - rect.tl.x;
	const int rectH = rect.br.y - rect.tl.y;
	int aantal=(( rectH - 1 ) / (ultraVncZ->maxSize(rectW * rectH) / rectW ) + 1 );
	m_queueEnable=false;
	if (m_use_lastrect && aantal>1 && m_allow_queue) {
		m_queueEnable=true;
		return 0;
	}
	// Return the number of rectangles needed to encode the given
	// update.  ( ZLIB_MAX_SIZE(rectW) / rectW ) is the number of lines in 
	// each maximum size rectangle.
	// When solid is enabled, most of the pixels are removed
	return (( rectH - 1 ) / (ultraVncZ->maxSize(rectW * rectH) / rectW ) + 1 );
}
//------------------------------------------------------------------
inline UINT vncEncodeZlib::EncodeRect(BYTE *source, VSocket *outConn, BYTE *dest, const rfb::Rect &rect, bool allow_queue)
{
	m_allow_queue = allow_queue;
	int  totalSize = 0;
	int  partialSize = 0;
	int  maxLines;
	int  linesRemaining;
	RECT partialRect;
	const int rectW = rect.br.x - rect.tl.x;
	const int rectH = rect.br.y - rect.tl.y;
	partialRect.right = rect.br.x;
	partialRect.left = rect.tl.x;
	partialRect.top = rect.tl.y;
	partialRect.bottom = rect.br.y;
	maxLines = ultraVncZ->maxSize(rectW * rectH) / rectW ;
	linesRemaining = rectH;
	while ( linesRemaining > 0 ) {
		int linesToComp;
		if ( maxLines < linesRemaining )
			linesToComp = maxLines;
		else
			linesToComp = linesRemaining;
		partialRect.bottom = partialRect.top + linesToComp;
		partialSize = EncodeOneRect( source, dest, partialRect,outConn);
		totalSize += partialSize;
		linesRemaining -= linesToComp;
		partialRect.top += linesToComp;
		if (( linesRemaining > 0 ) && ( partialSize > 0 )) {
			outConn->SendExactQueue( (char *)dest, partialSize );
			transmittedSize += partialSize;
		}
	}
	transmittedSize += partialSize;
	return partialSize;
}
//------------------------------------------------------------------
inline UINT vncEncodeZlib::EncodeOneRect(BYTE *source, BYTE *dest, const RECT &rect,VSocket *outConn)
{
	int totalCompDataLen = 0;
	UINT avail_in = 0;
	const int rectW = rect.right - rect.left;
	const int rectH = rect.bottom - rect.top;
	const int rawDataSize = (rectW*rectH*m_remoteformat.bitsPerPixel / 8);
	const int maxCompSize = (rawDataSize + (rawDataSize/100) + 8);
	// Create the rectangle header
	rfbFramebufferUpdateRectHeader *surh=(rfbFramebufferUpdateRectHeader *)dest;
	// Modif rdv@2002 - v1.1.x - Application Resize
	surh->r.x = (CARD16) rect.left-monitor_Offsetx;
	surh->r.y = (CARD16) rect.top-monitor_Offsety;
	surh->r.w = (CARD16) (rectW);
	surh->r.h = (CARD16) (rectH);
	surh->r.x = Swap16IfLE(surh->r.x);
	surh->r.y = Swap16IfLE(surh->r.y);
	surh->r.w = Swap16IfLE(surh->r.w);
	surh->r.h = Swap16IfLE(surh->r.h);
	surh->encoding = Swap32IfLE(m_use_zstd ? rfbEncodingZstd : rfbEncodingZlib);
	dataSize += ( rectW * rectH * m_remoteformat.bitsPerPixel) / 8;
	rectangleOverhead += sz_rfbFramebufferUpdateRectHeader;	
	// create a space big enough for the Zlib encoded pixels
	if (m_bufflen < rawDataSize) {
		if (m_buffer != NULL) {
			delete [] m_buffer;
			m_buffer = NULL;
		}
		m_buffer = new BYTE [rawDataSize+1000];
		if (m_buffer == NULL)
			return vncEncoder::EncodeRect(source, dest, rect);
		m_bufflen = rawDataSize+999;
	}

	avail_in = rawDataSize;
	Translate(source, m_buffer, rect);
	if (rawDataSize < ultraVncZ->minSize())
		return vncEncoder::EncodeRect(source, dest, rect);

	UINT newsize=0;
	if (rawDataSize < 1000 && m_queueEnable) {
		surh->encoding = Swap32IfLE(rfbEncodingRaw);
		memcpy(dest + sz_rfbFramebufferUpdateRectHeader, m_buffer, rawDataSize);
		AddToQueu(dest, sz_rfbFramebufferUpdateRectHeader + rawDataSize, outConn, 1);
		return 0;
	}
	surh->encoding = Swap32IfLE(m_use_zstd ? rfbEncodingZstd : rfbEncodingZlib);
	totalCompDataLen = ultraVncZ->compress(m_compresslevel, avail_in, maxCompSize, m_buffer, (dest + sz_rfbFramebufferUpdateRectHeader + sz_rfbZlibHeader));
	if (totalCompDataLen == 0)
		return vncEncoder::EncodeRect(source, dest, rect);	
	rfbZlibHeader *zlibh=(rfbZlibHeader *)(dest+sz_rfbFramebufferUpdateRectHeader);
	zlibh->nBytes = Swap32IfLE(totalCompDataLen);	
	// Update statistics
	encodedSize += sz_rfbZlibHeader + totalCompDataLen;
	rectangleOverhead += sz_rfbFramebufferUpdateRectHeader;
	return sz_rfbFramebufferUpdateRectHeader + sz_rfbZlibHeader + totalCompDataLen;
}
//------------------------------------------------------------------
void vncEncodeZlib::AddToQueu(BYTE *source,int sizerect,VSocket *outConn,int updatetype)
{
	if (m_Queuelen+sizerect>(MaxQueuebufflen)) 
		SendZlibrects(outConn);
	memcpy(m_Queuebuffer+m_Queuelen,source,sizerect);
	m_Queuelen+=sizerect;
	m_nNbRects++;
	if (updatetype==1) 
		must_be_zipped=true;
	if (m_nNbRects>50) 
		SendZlibrects(outConn);
}
//------------------------------------------------------------------
void vncEncodeZlib::SendZlibrects(VSocket *outConn)
{
	int NRects=m_nNbRects;
	const int rawDataSize = (m_Queuelen);
	UINT maxCompSize = (m_Queuelen + (m_Queuelen/100) + 8);
	if (NRects==0) return; // NO update
	if (m_nNbRects<3 && !must_be_zipped) {
		outConn->SendExactQueue( (char *)m_Queuebuffer, m_Queuelen); // 1 Small update
		m_nNbRects=0;
		m_Queuelen=0;
		encodedSize += m_Queuelen-sz_rfbFramebufferUpdateRectHeader;
		rectangleOverhead += sz_rfbFramebufferUpdateRectHeader;
		return;
	}
	m_nNbRects=0;
	m_Queuelen=0;
	must_be_zipped=false;
	maxCompSize = ultraVncZ->compress(m_compresslevel, rawDataSize, maxCompSize, m_Queuebuffer, m_QueueCompressedbuffer);
	if (maxCompSize == 0)
		return;
	int rawDataSize1=rawDataSize/65535;
	int rawDataSize2=rawDataSize%65535;
	rfbFramebufferUpdateRectHeader CacheRectsHeader;
	CacheRectsHeader.r.x = (CARD16)(NRects);
	CacheRectsHeader.r.y = (CARD16)(rawDataSize2);
	CacheRectsHeader.r.w = (CARD16)(rawDataSize1);
	CacheRectsHeader.r.x = Swap16IfLE(CacheRectsHeader.r.x);
	CacheRectsHeader.r.y = Swap16IfLE(CacheRectsHeader.r.y);
	CacheRectsHeader.r.w = Swap16IfLE(CacheRectsHeader.r.w);
 	CacheRectsHeader.r.h = 0;
	CacheRectsHeader.encoding = Swap32IfLE(m_use_zstd ? rfbEncodingQueueZstd :rfbEncodingQueueZip);
	// Format the ZlibHeader
	rfbZlibHeader CacheZipHeader;
	CacheZipHeader.nBytes = Swap32IfLE(maxCompSize);
	vnclog.Print(LL_INTINFO, VNCLOG("********QUEUEQUEUE********** %d %d %d\r\n"),maxCompSize,rawDataSize,NRects);
	outConn->SendExactQueue((char *)&CacheRectsHeader, sizeof(CacheRectsHeader));
	outConn->SendExactQueue((char *)&CacheZipHeader, sizeof(CacheZipHeader));
	outConn->SendExactQueue((char *)m_QueueCompressedbuffer, maxCompSize);
	// Update statistics
	encodedSize += sz_rfbZlibHeader + maxCompSize;
	rectangleOverhead += sz_rfbFramebufferUpdateRectHeader;
	transmittedSize += maxCompSize+sz_rfbFramebufferUpdateRectHeader+sz_rfbZlibHeader;
}
//------------------------------------------------------------------
void vncEncodeZlib::LastRect(VSocket *outConn)
{
	SendZlibrects(outConn);
}
//------------------------------------------------------------------
void vncEncodeZlib::set_use_zstd(bool enabled)
{
	ultraVncZ->set_use_zstd(enabled);
	vncEncoder::set_use_zstd(enabled);
}
//------------------------------------------------------------------