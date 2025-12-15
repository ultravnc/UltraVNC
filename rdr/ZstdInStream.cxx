// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//


#include "ZstdInStream.h"
#include "Exception.h"

using namespace rdr;

enum { DEFAULT_BUF_SIZE = 16384 };

ZstdInStream::ZstdInStream(int bufSize_)
	: underlying(0), bufSize(bufSize_ ? bufSize_ : DEFAULT_BUF_SIZE), offset(0),
	bytesIn(0)
{
	zstds = ZSTD_createDStream();
	unsigned int inSize = ZSTD_DStreamInSize();
	unsigned int outSize = ZSTD_DStreamOutSize();
	bufSize = inSize > outSize ? inSize : outSize;
	bufSize = bufSize_ > bufSize ? bufSize_ : bufSize;
	outBuffer = new ZSTD_outBuffer;
	inBuffer = new ZSTD_inBuffer;	
	ZSTD_initDStream(zstds);	
	ptr = end = start = new U8[bufSize];
}

ZstdInStream::~ZstdInStream()
{
	ZSTD_freeDStream(zstds);
	delete outBuffer;
	delete inBuffer;
}

void ZstdInStream::setUnderlying(InStream* is, int bytesIn_)
{
	underlying = is;
	bytesIn = bytesIn_;
	ptr = end = start;
}

int ZstdInStream::pos()
{
	return (int)(offset + ptr - start);
}

void ZstdInStream::reset()
{
	ptr = end = start;
	if (!underlying) return;

	while (bytesIn > 0) {
		decompress();
		end = start; // throw away any data
	}
	underlying = 0;
}

int ZstdInStream::overrun(int itemSize, int nItems)
{
	if (itemSize > bufSize)
		throw Exception("ZlibInStream overrun: max itemSize exceeded");
	if (!underlying)
		throw Exception("ZlibInStream overrun: no underlying stream");

	if (end - ptr != 0)
		memmove(start, ptr, end - ptr);

	offset += (int)(ptr - start);
	end -= ptr - start;
	ptr = start;

	while (end - ptr < itemSize) {
		decompress();
	}

	if (itemSize * nItems > end - ptr)
		nItems = (int)((end - ptr) / itemSize);

	return nItems;
}

// decompress() calls the decompressor once. Note that this won't necessarily
// generate any output data - it may just consume some input data.

void ZstdInStream::decompress()
{

	outBuffer->dst = (U8*)end;
	outBuffer->size = start + bufSize - end;
	outBuffer->pos = 0;

	underlying->check(1);
	inBuffer->src = underlying->getptr();
	inBuffer->size = underlying->getend() - underlying->getptr();
	inBuffer->pos = 0;

	if (inBuffer->size > (size_t)bytesIn)
		inBuffer->size = bytesIn;

	auto result = ZSTD_decompressStream(zstds, outBuffer, inBuffer);
	if (ZSTD_isError(result))
		throw Exception(ZSTD_getErrorName(result));

	bytesIn -= (int)(((U8*)inBuffer->src + inBuffer->pos) - (U8*)underlying->getptr());
	end = ((U8*)outBuffer->dst + outBuffer->pos);
	underlying->setptr((U8*)inBuffer->src + inBuffer->pos);
}
