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


// vncEncodeHexT object

// The vncEncodeHexT object uses a compression encoding to send rectangles
// to a client

class vncEncodeHexT;

#if !defined(_WINVNC_ENCODEHEXTILE)
#define _WINVNC_ENCODEHEXTILE
#pragma once

#include "vncencoder.h"

// Class definition

class vncEncodeHexT : public vncEncoder
{
// Fields
public:

// Methods
public:
	// Create/Destroy methods
	vncEncodeHexT();
	~vncEncodeHexT();

	virtual void Init();

	virtual UINT RequiredBuffSize(UINT width, UINT height);
	virtual UINT NumCodedRects(const rfb::Rect &rect);

	virtual UINT EncodeRect(BYTE *source, BYTE *dest, const rfb::Rect &rect);

protected:
	virtual UINT EncodeHextiles8(BYTE *source, BYTE *dest,
		int x, int y, int w, int h);
	virtual UINT EncodeHextiles16(BYTE *source, BYTE *dest,
		int x, int y, int w, int h);
	virtual UINT EncodeHextiles32(BYTE *source, BYTE *dest,
		int x, int y, int w, int h);

// Implementation
protected:
};

#endif // _WINVNC_ENCODEHEXTILE

