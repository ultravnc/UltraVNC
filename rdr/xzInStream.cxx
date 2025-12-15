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


#ifdef _XZ
#include "xzInStream.h"
#include "Exception.h"
#include "assert.h"

using namespace rdr;

enum { DEFAULT_BUF_SIZE = 0x10000 };

xzInStream::xzInStream(int bufSize_)
  : underlying(0), bufSize(bufSize_ ? bufSize_ : DEFAULT_BUF_SIZE), offset(0),
    bytesIn(0), ls(NULL)
{
	ptr = end = start = new U8[bufSize];
}

xzInStream::~xzInStream()
{
  delete [] start;
  if (ls) {
	  lzma_end(ls);
	  delete ls;
  }
}

void xzInStream::ensure_stream_codec()
{
	if (ls) return;
	
	ls = new lzma_stream;

	memset(ls, 0, sizeof(lzma_stream));
	lzma_ret rc = LZMA_OK;

	rc = lzma_stream_decoder(ls, UINT64_MAX, LZMA_TELL_UNSUPPORTED_CHECK);
	if (rc != LZMA_OK) {
		fprintf (stderr, "lzma_stream_decoder error: %d\n", (int) rc);
		throw Exception("lzmaOutStream: lzma_stream_decoder failed");
		return;
	}
}

void xzInStream::setUnderlying(InStream* is, int bytesIn_)
{
  underlying = is;
  bytesIn = bytesIn_;
  ptr = end = start;
}

int xzInStream::pos()
{
  return (int)(offset + ptr - start);
}

void xzInStream::reset()
{
  ensure_stream_codec();

  ptr = end = start;
  if (!underlying) return;

  while (bytesIn > 0) {
    decompress();
    end = start; // throw away any data
  }
  underlying = 0;
}

int xzInStream::overrun(int itemSize, int nItems)
{
  ensure_stream_codec();

  if (itemSize > bufSize)
    throw Exception("xzInStream overrun: max itemSize exceeded");
  if (!underlying)
    throw Exception("xzInStream overrun: no underlying stream");

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

void xzInStream::decompress()
{
	ls->next_out = (U8*)end;
	ls->avail_out = start + bufSize - end;

	if (bytesIn > 0) {
		underlying->check(1);
	} else {
		assert(false);
	}
	ls->next_in = (U8*)underlying->getptr();
	ls->avail_in = underlying->getend() - underlying->getptr();
	if ((int)ls->avail_in > bytesIn)
		ls->avail_in = bytesIn;

	lzma_ret rc = lzma_code(ls, LZMA_RUN);
	if ((rc != LZMA_OK) && (rc != LZMA_STREAM_END)) {
		fprintf (stderr, "lzma_code decompress error: %d\n", (int) rc);
		throw Exception("lzmaOutStream: decompress failed");
	}

	bytesIn -= (int)(ls->next_in - underlying->getptr());
	end = ls->next_out;
	underlying->setptr(ls->next_in);
}
#endif
