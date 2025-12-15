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
// xzOutStream streams to a compressed data stream (underlying), compressing
// with zlib on the fly.
//
#ifdef _XZ
#ifndef __RDR_xzOutStream_H__
#define __RDR_xzOutStream_H__

#include "OutStream.h"

#define LZMA_API_STATIC
#ifndef _VS2008
#include <stdint.h>
#endif
#ifdef _XZ
#ifdef _VCPKG
#include  <lzma.h>
#else
#include "../xz/src/liblzma/api/lzma.h"
#endif
#endif

namespace rdr {

  class xzOutStream : public OutStream {

  public:

    // adzm - 2010-07 - Custom compression level
    xzOutStream(OutStream* os=0, int bufSize=0);
    virtual ~xzOutStream();

	void SetCompressLevel(int compression);

    void setUnderlying(OutStream* os);
    void flush();
    int length();

  private:

    void ensure_stream_codec();

    int overrun(int itemSize, int nItems);

    OutStream* underlying;
    int bufSize;
    int offset;
#ifdef _XZ
	lzma_stream* ls;
	lzma_options_lzma ls_options;
#endif
    U8* start;
  };

} // end of namespace rdr

#endif
#endif
