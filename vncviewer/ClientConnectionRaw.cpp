// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"

void ClientConnection::ReadRawRect(rfbFramebufferUpdateRectHeader *pfburh) {

	UINT numpixels = pfburh->r.w * pfburh->r.h;
    // this assumes at least one byte per pixel. Naughty.
	UINT numbytes = numpixels * m_minPixelBytes;
	// Security Check
	if (numbytes > 106000000)
		goto error;
	// Read in the whole thing
    CheckBufferSize(numbytes);
	ReadExact(m_netbuf, numbytes);
	if (m_DIBbits)
	{
	omni_mutex_lock l(m_bitmapdcMutex);
	ConvertAll_secure(pfburh->r.w,pfburh->r.h,pfburh->r.x, pfburh->r.y,m_myFormat.bitsPerPixel/8,(BYTE *)m_netbuf,(BYTE *)m_DIBbits,m_si.framebufferWidth, numbytes, m_si.framebufferHeight);
	}
	return;
	error:
		assert(true);
}

