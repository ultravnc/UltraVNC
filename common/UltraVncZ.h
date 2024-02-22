/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
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
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


#if !defined(_WINVNC_VNCZ)
#define _WINVNC_VNCZ
#pragma once
#include "../zlib/zlib.h"
#include "../zstd/lib/zstd.h"


class UltraVncZ 
{
public:
	UltraVncZ();
	~UltraVncZ();
	UINT compressZstd(int compresslevel, UINT avail_in, UINT avail_out, BYTE * next_in, BYTE *next_out);
	UINT compressZlib(int compresslevel, UINT avail_in, UINT avail_out, BYTE * next_in, BYTE *next_out);
	UINT compress(int compresslevel, UINT avail_in, UINT avail_out, BYTE * next_in, BYTE *next_out);

	int decompressZstd(UINT &avail_in, UINT &avail_out, BYTE * next_in, BYTE *next_out);
	int decompressZlib(UINT &avail_in, UINT &avail_out, BYTE * next_in, BYTE *next_out);
	int decompress(UINT &avail_in, UINT &avail_out, BYTE * next_in, BYTE *next_out, bool zstd);

	UINT maxSize(UINT size);
	UINT minSize();
	void set_use_zstd(bool use_zstd);

	void endInflateStream(bool zstd);
protected:
	z_stream_s  compStream;
	z_stream decompStream;
	bool compStreamInitedZlib;
	bool decompStreamInitedZlib;
	bool compStreamInitedZstd;
	bool decompStreamInitedZstd;
	UINT MAX_SIZE;
	bool use_zstd;

	ZSTD_CStream* cstream;
	ZSTD_outBuffer* outBufferC;
	ZSTD_inBuffer* inBufferC;

	ZSTD_DStream* dstream;
	ZSTD_outBuffer* outBufferD;
	ZSTD_inBuffer* inBufferD;

	int compresslevel;
	/*void createCDict(int cLevel, ZSTD_CStream* cstream);
	void createDDict();
	UINT compressZstd_usingCDict(int compresslevel, UINT avail_in, UINT avail_out, BYTE * next_in, BYTE *next_out);
	int decompressZstd_usingCDict(UINT &avail_in, UINT &avail_out, BYTE * next_in, BYTE *next_out);*/
};

#endif // _WINVNC_VNCZ

