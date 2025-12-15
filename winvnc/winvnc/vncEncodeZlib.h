// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


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

