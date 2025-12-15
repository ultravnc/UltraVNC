// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2000 Tridia Corporation. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


// vncEncodeZlibHex object

// The vncEncodeZlibHex object uses a compression encoding to send rectangles
// to a client. As with the hextile encoding, all rectangles are broken down
// into a matrix of 16x16 (or smaller at bottom/right) tiles, which are 
// individually encoded with a subencoding mechanism. This encoding addds
// the ability to apply Zlib compression to the raw and other hextile
// subencodings.

class vncEncodeZlibHex;

#if !defined(_WINVNC_ENCODEZLIBHEX)
#define _WINVNC_ENCODEZLIBHEX
#pragma once

#include "vncencoder.h"
#include "lzo/minilzo.h"

// Minimum zlib rectangle size in bytes. Anything smaller will
// not compress well due to overhead.
// temp change lzo
//#define VNC_ENCODE_ZLIBHEX_MIN_COMP_SIZE (17)
#define VNC_ENCODE_ZLIBHEX_MIN_COMP_SIZE (64)
// Flag used to mark our compressors as uninitialized.
#define ZLIBHEX_COMP_UNINITED (-1)

// Size of the smallest update portion sent independently across
// the network. This encoder can transmit partial updates to
// improve latency issues with performance.
#define VNC_ENCODE_ZLIBHEX_MIN_DATAXFER (1400)

// Class definition
class UltraVncZ;

class vncEncodeZlibHex : public vncEncoder
{
// Fields
public:

// Methods
public:
	// Create/Destroy methods
	vncEncodeZlibHex();
	~vncEncodeZlibHex();
	void Init();
	virtual const char* GetEncodingName() { return "ZlibHex"; }
	virtual UINT RequiredBuffSize(UINT width, UINT height);
	virtual UINT NumCodedRects(RECT &rect);
	// virtual UINT EncodeRect(BYTE *source, BYTE *dest, const RECT &rect);
	virtual UINT EncodeRect(BYTE *source, VSocket *outConn, BYTE *dest, const RECT &rect);
	virtual void LastRect(VSocket *outConn);
	virtual void AddToQueu(BYTE *source,int size,VSocket *outConn);
	virtual void SendZlibHexrects(VSocket *outConn);
	virtual void set_use_zstd(bool enabled);

protected:
	virtual UINT zlibCompress(BYTE *from_buf, BYTE *to_buf, UINT length, UltraVncZ *compressor);
	virtual UINT EncodeHextiles8(BYTE *source, BYTE *dest,
		VSocket *outConn, int x, int y, int w, int h);
	virtual UINT EncodeHextiles16(BYTE *source, BYTE *dest,
		VSocket *outConn, int x, int y, int w, int h);
	virtual UINT EncodeHextiles32(BYTE *source, BYTE *dest,
		VSocket *outConn, int x, int y, int w, int h);

// Implementation
protected:
	BYTE		      *m_buffer;
	int			       m_bufflen;	
	//bool lzo;
	BYTE			  *m_Queuebuffer;
	int					m_Queuelen;
	int					MaxQueuebufflen;
	UltraVncZ   *ultraVncZRaw;
	UltraVncZ   *ultraVncZEncoded;
};

#endif // _WINVNC_ENCODEHEXTILE

