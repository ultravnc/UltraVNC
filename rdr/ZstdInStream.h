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


//
// ZstdInStream streams from a compressed data stream ("underlying"),
// decompressing with zlib or zstd on the fly.
//
#ifndef __RDR_ZSTDINSTREAM_H__
#define __RDR_ZSTDINSTREAM_H__

#pragma once

#include "InStream.h"
#ifdef _VCPKG
#include <zstd.h>
#else
#include "../zstd/lib/zstd.h"
#endif

struct z_stream_s;

namespace rdr {

  class ZstdInStream : public InStream {

  public:

    ZstdInStream(int bufSize=0);
    virtual ~ZstdInStream();

    void setUnderlying(InStream* is, int bytesIn);
    void reset();
    int pos();

  private:

    int overrun(int itemSize, int nItems);
    void decompress();

    InStream* underlying;
    int bufSize;
    int offset;
    int bytesIn;
    U8* start;

	ZSTD_outBuffer* outBuffer;
	ZSTD_inBuffer* inBuffer;
	ZSTD_DStream* zstds;

  };
} // end of namespace rdr

#endif
