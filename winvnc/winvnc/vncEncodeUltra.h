// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


// vncEncodeUltra object

// The vncEncodeUltra object uses a Ultra based compression encoding to send rectangles
// to a client

class vncEncodeUltra;

#if !defined(_WINVNC_EncodeULTRA)
#define _WINVNC_EncodeULTRA
#pragma once

#include "vncencoder.h"
#include "lzo/minilzo.h"

// Minimum Ultra rectangle size in bytes. Anything smaller will
// not compress well due to overhead.
#define VNC_ENCODE_ULTRA_MIN_COMP_SIZE (32)

// Set maximum Ultra rectangle size in pixels. Always allow at least
// two scan lines.
#define Ultra_MAX_RECT_SIZE (16*1024)
#define Ultra_MAX_SIZE(min) ((( min * 2 ) > Ultra_MAX_RECT_SIZE ) ? ( min * 2 ) : Ultra_MAX_RECT_SIZE )
#define SOLID_COLOR	0 // 1 color
#define MONO_COLOR	1 //2 colors
#define MULTI_COLOR	2 // >2 colors
#define PURE_Ultra	3 
#define	XOR_SEQUENCE 4 //XOR previous image


// Class definition

class vncEncodeUltra : public vncEncoder
{
// Fields
public:

// Methods
public:
	// Create/Destroy methods
	vncEncodeUltra();
	~vncEncodeUltra();

	virtual void Init();
	virtual const char* GetEncodingName() { return "Ultra"; }

	virtual UINT RequiredBuffSize(UINT width, UINT height);
	virtual UINT NumCodedRects(const rfb::Rect &rect);

	virtual UINT EncodeRect(BYTE *source,VSocket *outConn, BYTE *dest, const rfb::Rect &rect);
	virtual UINT EncodeOneRect(BYTE *source ,BYTE *dest, const RECT &rect,VSocket *outConn);

	virtual void LastRect(VSocket *outConn);
	virtual void AddToQueu(BYTE *source,int size,VSocket *outConn,int must_be_zipped);
	virtual void AddToQueu2(BYTE *source,int size,VSocket *outConn,int must_be_zipped);
	virtual void SendUltrarects(VSocket *outConn);

	void EnableQueuing(BOOL fEnable){ m_queueEnable = fEnable; };

// Implementation
protected:
	BYTE		      *m_buffer;
	BYTE			  *m_Queuebuffer;
	BYTE			  *m_QueueCompressedbuffer;
	int			       m_bufflen;
	int totalraw;

	int				   m_Queuebufflen;
	int				   MaxQueuebufflen;
	int				   m_Queuelen;
	int				   m_nNbRects;
	int				   must_be_zipped;
	BOOL				m_queueEnable;

	bool				lzo;
	lzo_uint out_len;
};

#endif // _WINVNC_EncodeUltra

