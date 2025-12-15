// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


// vncEncodeCoRRE object

// The vncEncodeCoRRE object uses a compression encoding to send rectangles
// to a client

class vncEncodeCoRRE;

#if !defined(_WINVNC_ENCODECORRRE)
#define _WINVNC_ENCODECORRE
#pragma once

#include "vncencoder.h"

// Class definition

class vncEncodeCoRRE : public vncEncoder
{
// Fields
public:

// Methods
public:
	// Create/Destroy methods
	vncEncodeCoRRE();
	~vncEncodeCoRRE();

	virtual void Init();

	virtual UINT RequiredBuffSize(UINT width, UINT height);
	virtual UINT NumCodedRects(const rfb::Rect &rect);

	virtual UINT EncodeRect(BYTE *source, BYTE *dest, const rfb::Rect &rect);
	virtual void SetCoRREMax(BYTE width, BYTE height);
protected:
	virtual UINT InternalEncodeRect(BYTE *source, BYTE *dest, const rfb::Rect &rect);
	virtual UINT EncodeSmallRect(BYTE *source, BYTE *dest, const rfb::Rect &rect);

// Implementation
protected:
	BYTE		*m_buffer;
	int			m_bufflen;

	// Maximum height & width for CoRRE
	UINT		m_maxwidth;
	UINT		m_maxheight;

	// Last-update stats for CoRRE
	UINT		m_encodedbytes, m_rectbytes;
	UINT		m_lastencodedbytes, m_lastrectbytes;
	int			m_maxadjust;
	int			m_threshold;
	BOOL		m_statsready;
};

#endif // _WINVNC_ENCODECORRE

