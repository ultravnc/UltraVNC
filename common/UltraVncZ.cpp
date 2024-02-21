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


#include "stdhdrs.h"
#include "UltraVncZ.h"
#include <stdlib.h> 
#include <sys/stat.h> 

UltraVncZ::UltraVncZ()
{
	compStreamInitedZlib = false;
	decompStreamInitedZlib = false;
	compStreamInitedZstd = false;
	decompStreamInitedZstd = false;
	use_zstd = false;
}
void UltraVncZ::set_use_zstd(bool use_zstd)
{
	this->use_zstd = use_zstd;
	if (use_zstd)
		MAX_SIZE = ZSTD_CStreamInSize();
	else
		MAX_SIZE = 8192;
}

UltraVncZ::~UltraVncZ()
{
	if (compStreamInitedZlib)
		deflateEnd(&compStream);
	if (decompStreamInitedZlib)
		inflateEnd(&decompStream);
	if (compStreamInitedZstd) {
		ZSTD_freeCStream(cstream);
		free(outBufferC);
		free(inBufferC);
	}
	if (decompStreamInitedZstd) {
		ZSTD_freeDStream(dstream);
		free(outBufferD);
		free(inBufferD);
	}
}

void UltraVncZ::endInflateStream(bool zstd)
{
	if (zstd && compStreamInitedZstd) {
		ZSTD_freeCStream(cstream);
		free(outBufferC);
		free(inBufferC);
		compStreamInitedZstd = false;
	}

	if (!zstd && decompStreamInitedZlib) {
		inflateEnd(&decompStream);
		decompStreamInitedZlib = false;
	}
}


UINT UltraVncZ::compress(int compresslevel, UINT avail_in, UINT avail_out, BYTE * next_in, BYTE *next_out)
{
	if (use_zstd)
		return compressZstd(compresslevel, avail_in, avail_out, next_in, next_out);
	else
		return compressZlib(compresslevel, avail_in, avail_out, next_in, next_out);
}

UINT UltraVncZ::compressZlib (int compresslevel, UINT avail_in, UINT avail_out, BYTE * next_in, BYTE *next_out)
{
	compStream.next_in = next_in;
	compStream.avail_out = avail_out;
	compStream.next_out = next_out;
	compStream.avail_in = avail_in;

	int previousTotalOut;
	int deflateResult;
	if (!compStreamInitedZlib) {
		compStream.total_in = 0;
		compStream.total_out = 0;
		compStream.zalloc = Z_NULL;
		compStream.zfree = Z_NULL;
		compStream.opaque = Z_NULL;
		compStream.data_type = Z_BINARY;
		deflateResult = deflateInit2(&compStream, compresslevel, Z_DEFLATED, MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
		if (deflateResult != Z_OK)
			return 0;
		compStreamInitedZlib = true;
		this->compresslevel = compresslevel;
	}
	previousTotalOut = compStream.total_out;
	if (this->compresslevel != compresslevel) {
		deflateParams(&compStream, compresslevel, Z_DEFAULT_STRATEGY);
		this->compresslevel = compresslevel;
	}
	deflateResult = deflate(&compStream, Z_SYNC_FLUSH);
	if (deflateResult != Z_OK)
		return 0;
	return compStream.total_out - previousTotalOut;
}

UINT UltraVncZ::compressZstd(int compresslevel, UINT avail_in, UINT avail_out, BYTE * next_in, BYTE *next_out)
{
	compresslevel = compresslevel - 7;
	unsigned int rc = 0;
	if (!compStreamInitedZstd) {		
		cstream = ZSTD_createCStream();
		if (cstream ==NULL)			
			return 0;
		if (ZSTD_isError(ZSTD_initCStream(cstream, compresslevel)))
			return 0;
		if (ZSTD_isError(ZSTD_CCtx_setParameter(cstream, ZSTD_c_strategy, ZSTD_fast)))
			return 0;
		outBufferC = new ZSTD_outBuffer;
		inBufferC = new ZSTD_inBuffer;
		compStreamInitedZstd = true;
		this->compresslevel = compresslevel;
	}

	inBufferC->src = next_in;
	inBufferC->size = avail_in;
	inBufferC->pos = 0;
	outBufferC->dst = next_out;
	outBufferC->size =avail_out;
	outBufferC->pos = 0;
	if (this->compresslevel != compresslevel) {
		ZSTD_CCtx_setParameter(cstream, ZSTD_c_compressionLevel, compresslevel);
		this->compresslevel = compresslevel;
	}
	rc = ZSTD_compressStream2(cstream, outBufferC, inBufferC, ZSTD_e_flush);
	if (ZSTD_isError(rc))
		return 0;
	return outBufferC->pos;
}

int UltraVncZ::decompress(UINT &avail_in, UINT &avail_out, BYTE * next_in, BYTE *next_out, bool zstd)
{
	if (zstd)
		return decompressZstd(avail_in, avail_out, next_in, next_out);
	else
		return decompressZlib(avail_in, avail_out, next_in, next_out);
}

int UltraVncZ::decompressZlib(UINT &avail_in, UINT &avail_out, BYTE * next_in, BYTE *next_out)
{
	int inflateResult;
	decompStream.next_in = next_in;
	decompStream.avail_in = avail_in;
	decompStream.next_out = next_out;
	decompStream.avail_out = avail_out;
	decompStream.data_type = Z_BINARY;

	// Insure the inflator is initialized
	if (decompStreamInitedZlib == false) {
		decompStream.total_in = 0;
		decompStream.total_out = 0;
		decompStream.zalloc = Z_NULL;
		decompStream.zfree = Z_NULL;
		decompStream.opaque = Z_NULL;

		inflateResult = inflateInit(&decompStream);
		if (inflateResult != Z_OK)
			return  inflateResult;
		decompStreamInitedZlib = true;
	}
	// Decompress screen data
	int result = inflate(&decompStream, Z_SYNC_FLUSH);
	avail_in = decompStream.avail_in;
	avail_out = decompStream.avail_out;
	return result;
}

int UltraVncZ::decompressZstd(UINT &avail_in, UINT &avail_out, BYTE * next_in, BYTE *next_out)
{
	unsigned int rc = 0;
	if (!decompStreamInitedZstd) {
		dstream = ZSTD_createDStream();
		if (dstream == NULL)
			return 0;
		if (ZSTD_isError(ZSTD_initDStream(dstream)))
			return 0;
		outBufferD = new ZSTD_outBuffer;
		inBufferD = new ZSTD_inBuffer;
		decompStreamInitedZstd = true;
	}

	inBufferD->src = next_in;
	inBufferD->size = avail_in;
	inBufferD->pos = 0;
	outBufferD->dst = next_out;
	outBufferD->size = avail_out;
	outBufferD->pos = 0;
	rc = ZSTD_decompressStream(dstream, outBufferD, inBufferD);
	avail_in = inBufferD->size;
	avail_out = outBufferD->size - outBufferD->pos;
	if (!ZSTD_isError(rc))
		return Z_OK;
	else
		return Z_ERRNO;
}

UINT UltraVncZ::maxSize(UINT size)
{	
	return (size > MAX_SIZE) ? size : MAX_SIZE;
}

UINT UltraVncZ::minSize()
{
	if (use_zstd)
		return 1000;
	else
		return 25;
}

/*void UltraVncZ::createCDict(int cLevel, ZSTD_CStream* cstream)
{
	char file[MAX_PATH];
	GetModuleFileName(NULL, file, MAX_PATH);
	char* p = strrchr(file, '\\');
	*p = '\0';
	strcat_s(file, "\\dict");
	struct stat st;
	if (stat(file, &st) != 0)
		goto error;
	off_t const fileSize = st.st_size;
	size_t const size = (size_t)fileSize;
	if ((fileSize < 0) || (fileSize != (off_t)size))
		goto error;
	void* const buffer = malloc(fileSize);

	FILE* const inFile = fopen(file, "rb");
	size_t const readSize = fread(buffer, 1, fileSize, inFile);
	if (readSize != (size_t)fileSize)
		goto error;
	fclose(inFile);
	ZSTD_CDict* const cdict = ZSTD_createCDict(buffer, fileSize, cLevel);
	free(buffer);
	size_t t =  ZSTD_CCtx_loadDictionary(cstream, cdict, fileSize);
	return;
error:
	//Adding a NULL (or 0-size) dictionary invalidates any previous dictionary,
	// meaning "return to no-dictionary mode".
	ZSTD_CCtx_loadDictionary(cstream, NULL, fileSize);
}

void UltraVncZ::createDDict()
{
	char file[MAX_PATH];
	GetModuleFileName(NULL, file, MAX_PATH);
	char* p = strrchr(file, '\\');
	*p = '\0';
	strcat_s(file, "\\dict");
	struct stat st;
	if (stat(file, &st) != 0)
		goto error;
	off_t const fileSize = st.st_size;
	size_t const size = (size_t)fileSize;
	if ((fileSize < 0) || (fileSize != (off_t)size))
		goto error;
	void* const buffer = malloc(fileSize);

	FILE* const inFile = fopen(file, "rb");
	size_t const readSize = fread(buffer, 1, fileSize, inFile);
	if (readSize != (size_t)fileSize)
		goto error;
	fclose(inFile);
	ZSTD_DDict* const ddict = ZSTD_createDDict(buffer, fileSize);
	free(buffer);
	size_t t = ZSTD_DCtx_loadDictionary(dstream, ddict, fileSize);
	return;
error:
	//Adding a NULL (or 0-size) dictionary invalidates any previous dictionary,
	// meaning "return to no-dictionary mode".
	ZSTD_DCtx_loadDictionary(dstream, NULL, fileSize);
}

UINT UltraVncZ::compressZstd_usingCDict(int compresslevel, UINT avail_in, UINT avail_out, BYTE * next_in, BYTE *next_out)
{
	compresslevel = compresslevel - 7;
	unsigned int rc = 0;
	if (!compStreamInitedZstd) {
		cstream = ZSTD_createCStream();
		if (cstream == NULL)
			return 0;		
		if (ZSTD_isError(ZSTD_initCStream(cstream, compresslevel)))
			return 0;
		ZSTD_CCtx_setParameter(cstream, ZSTD_c_strategy, ZSTD_fast);
		createCDict(compresslevel, cstream);
		outBufferC = new ZSTD_outBuffer;
		inBufferC = new ZSTD_inBuffer;
		compStreamInitedZstd = true;
		this->compresslevel = compresslevel;
	}

	inBufferC->src = next_in;
	inBufferC->size = avail_in;
	inBufferC->pos = 0;
	outBufferC->dst = next_out;
	outBufferC->size = avail_out;
	outBufferC->pos = 0;
	if (this->compresslevel != compresslevel) {
		ZSTD_CCtx_setParameter(cstream, ZSTD_c_compressionLevel, compresslevel);
		this->compresslevel = compresslevel;
	}
	rc = ZSTD_compressStream2(cstream, outBufferC, inBufferC, ZSTD_e_flush);
	if (ZSTD_isError(rc))
		return 0;
	return outBufferC->pos;
}

int UltraVncZ::decompressZstd_usingCDict(UINT &avail_in, UINT &avail_out, BYTE * next_in, BYTE *next_out)
{
	unsigned int rc = 0;
	if (!decompStreamInitedZstd) {
		dstream = ZSTD_createDStream();
		if (dstream == NULL)
			return 0;		
		if (ZSTD_isError(ZSTD_initDStream(dstream)))
			return 0;
		createDDict();
		outBufferD = new ZSTD_outBuffer;
		inBufferD = new ZSTD_inBuffer;
		decompStreamInitedZstd = true;
	}

	inBufferD->src = next_in;
	inBufferD->size = avail_in;
	inBufferD->pos = 0;
	outBufferD->dst = next_out;
	outBufferD->size = avail_out;
	outBufferD->pos = 0;
	rc = ZSTD_decompressStream(dstream, outBufferD, inBufferD);
	avail_in = inBufferD->size;
	avail_out = outBufferD->size - outBufferD->pos;
	if (!ZSTD_isError(rc))
		return Z_OK;
	else
		return Z_ERRNO;
}*/


