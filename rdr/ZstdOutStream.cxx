/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
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


#include "ZstdOutStream.h"
#include "Exception.h"
#include <stdlib.h> 

using namespace rdr;

enum { DEFAULT_BUF_SIZE = 16384 };


// adzm - 2010-07 - Custom compression level
ZstdOutStream::ZstdOutStream(OutStream* os, int bufSize_, int compressionLevel)
  : underlying(os), bufSize(bufSize_ ? bufSize_ : DEFAULT_BUF_SIZE), offset(0)
{
  zstds = ZSTD_createCStream();
  unsigned int inSize = ZSTD_CStreamInSize();
  unsigned int outSize = ZSTD_CStreamOutSize();
  bufSize = inSize > outSize ? inSize : outSize;
  bufSize = bufSize_ > bufSize ? bufSize_ : bufSize;
  ZSTD_initCStream(zstds, ZSTD_CLEVEL_DEFAULT);
  outBuffer = new ZSTD_outBuffer;
  inBuffer = new ZSTD_inBuffer;
  ptr = start = new U8[bufSize];
  end = start + bufSize;
}

ZstdOutStream::~ZstdOutStream()
{
  ZSTD_freeCStream(zstds);
  delete outBuffer;
  delete inBuffer;
  delete[] start;
}

void ZstdOutStream::setUnderlying(OutStream* os)
{
  underlying = os;
}

int ZstdOutStream::length()
{
  return (int)(offset + ptr - start);
}


void ZstdOutStream::flush()
{	
	inBuffer->src = start;
	inBuffer->size = ptr - start;
	inBuffer->pos = 0;
	unsigned int rc = 0;

	while (inBuffer->size != 0) {
		do {
			underlying->check(1);
			outBuffer->dst = underlying->getptr();
			outBuffer->size = underlying->getend() - underlying->getptr();
			outBuffer->pos = 0;

			rc = ZSTD_compressStream2(zstds, outBuffer, inBuffer, ZSTD_e_flush);
			if (ZSTD_isError(rc)) {
				auto error = ZSTD_getErrorName(rc);
				throw Exception(error);
			}
			inBuffer->src = (U8*)inBuffer->src + inBuffer->pos;
			inBuffer->size -= inBuffer->pos;
			inBuffer->pos = 0;
			outBuffer->dst = (U8*)outBuffer->dst + outBuffer->pos;
			outBuffer->size -= outBuffer->pos;
			outBuffer->pos = 0;

			underlying->setptr((U8*)outBuffer->dst);
		} while (outBuffer->size == 0);
	}

	if (rc != 0)
		__debugbreak();	
	offset += (int)(ptr - start);
	ptr = start;
}

int ZstdOutStream::overrun(int itemSize, int nItems)
{
	//    fprintf(stderr,"ZstdOutStream overrun\n");
	if (itemSize > bufSize)
		throw Exception("ZstdOutStream overrun: max itemSize exceeded");

	while (end - ptr < itemSize) {
		inBuffer->src = start;
		inBuffer->size = ptr - start;
		inBuffer->pos = 0;

		do {
			underlying->check(1);
			outBuffer->dst = underlying->getptr();
			outBuffer->size = underlying->getend() - underlying->getptr();
			outBuffer->pos = 0;

			auto rc = ZSTD_compressStream(zstds, outBuffer, inBuffer);
			if (ZSTD_isError(rc)) {
				auto error = ZSTD_getErrorName(rc);
				throw Exception(error);
			}
			inBuffer->src = (U8*)inBuffer->src + inBuffer->pos;
			inBuffer->size -= inBuffer->pos;
			inBuffer->pos = 0;
			outBuffer->dst = (U8*)outBuffer->dst + outBuffer->pos;
			outBuffer->size -= outBuffer->pos;
			outBuffer->pos = 0;

			underlying->setptr((U8*)outBuffer->dst);
		} while (outBuffer->size == 0);
		// output buffer not full
		if (inBuffer->size == 0) {
			offset += (int)(ptr - start);
			ptr = start;
		}
		else {
			// but didn't consume all the data?  try shifting what's left to the
			// start of the buffer.
			fprintf(stderr, "z out buf not full, but in data not consumed\n");
			memmove(start, inBuffer->src, ptr - (U8*)inBuffer->src);
			offset += (int)((U8*)inBuffer->src - start);
			ptr -= (U8*)inBuffer->src - start;
		}
	}
	if (itemSize * nItems > end - ptr)
		nItems = (int)((end - ptr) / itemSize);
	return nItems;
}
