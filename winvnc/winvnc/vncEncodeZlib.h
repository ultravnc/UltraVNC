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

class vncEncodeZlib;

#if !defined(_WINVNC_ENCODEZLIB)
#define _WINVNC_ENCODEZLIB
#pragma once

#include "vncencoder.h"

class UltraVncZ;
class vncEncodeZlib : public vncEncoder
{
// Fields
public:

// Methods
public:
	// Create/Destroy methods
	vncEncodeZlib();
	~vncEncodeZlib();

	virtual void Init();
	virtual const char* GetEncodingName() { return "Zlib"; }

	virtual UINT RequiredBuffSize(UINT width, UINT height);
	virtual UINT NumCodedRects(const rfb::Rect &rect);

	virtual UINT EncodeRect(BYTE *source, VSocket *outConn, BYTE *dest, const rfb::Rect &rect, bool allow_queue);
	virtual UINT EncodeOneRect(BYTE *source, BYTE *dest, const RECT &rect,VSocket *outConn);

	virtual void LastRect(VSocket *outConn);
	virtual void AddToQueu(BYTE *source,int size,VSocket *outConn,int must_be_zipped);
	virtual void SendZlibrects(VSocket *outConn);
	virtual void set_use_zstd(bool enabled);


// Implementation
protected:
	BYTE	*m_buffer;
	BYTE	*m_Queuebuffer;
	BYTE	*m_QueueCompressedbuffer;
	int		m_bufflen;
	int		totalraw;

	int		m_Queuebufflen;
	int		MaxQueuebufflen;
	int		m_Queuelen;
	int		m_nNbRects;
	int		must_be_zipped;
	BOOL	m_queueEnable;
	bool	m_allow_queue;
	int		Firstrun;
	UltraVncZ   *ultraVncZ;
};

#endif // _WINVNC_ENCODEZLIB

