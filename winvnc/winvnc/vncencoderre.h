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


// vncEncodeRRE object

// The vncEncodeRRE object uses a compression encoding to send rectangles
// to a client

class vncEncodeRRE;

#if !defined(_WINVNC_ENCODERRRE)
#define _WINVNC_ENCODERRE
#pragma once

#include "vncencoder.h"

// Class definition

class vncEncodeRRE : public vncEncoder
{
// Fields
public:

// Methods
public:
	// Create/Destroy methods
	vncEncodeRRE();
	~vncEncodeRRE();

	virtual void Init();

	virtual UINT RequiredBuffSize(UINT width, UINT height);
	virtual UINT NumCodedRects(const rfb::Rect &rect);

	virtual UINT EncodeRect(BYTE *source, BYTE *dest, const rfb::Rect &rect);

// Implementation
protected:
	BYTE		*m_buffer;
	int			m_bufflen;
};

#endif // _WINVNC_ENCODERRE

