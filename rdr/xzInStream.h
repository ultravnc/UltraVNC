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
// xzInStream streams from a compressed data stream ("underlying"),
// decompressing with zlib on the fly.
//
#ifdef _XZ
#ifndef __RDR_xzInStream_H__
#define __RDR_xzInStream_H__

#pragma once

#include "InStream.h"

#define LZMA_API_STATIC
#include <stdint.h>

#ifdef _VCPKG
#include  <lzma.h>
#else
#include "../xz/src/liblzma/api/lzma.h"
#endif

namespace rdr {

  class xzInStream : public InStream {

  public:

    xzInStream(int bufSize=0);
    virtual ~xzInStream();

    void setUnderlying(InStream* is, int bytesIn);
    void reset();
    int pos();

  private:

    void ensure_stream_codec();

    int overrun(int itemSize, int nItems);
    void decompress();

    InStream* underlying;
    int bufSize;
    int offset;
    lzma_stream* ls;
    int bytesIn;
    U8* start;
  };

} // end of namespace rdr

#endif
#endif
