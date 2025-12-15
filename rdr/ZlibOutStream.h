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
// ZlibOutStream streams to a compressed data stream (underlying), compressing
// with zlib on the fly.
//

#ifndef __RDR_ZLIBOUTSTREAM_H__
#define __RDR_ZLIBOUTSTREAM_H__

#include "OutStream.h"

struct z_stream_s;

namespace rdr {

  class ZlibOutStream : public OutStream {

  public:

    // adzm - 2010-07 - Custom compression level
    ZlibOutStream(OutStream* os=0, int bufSize=0, int compressionLevel=-1); // Z_DEFAULT_COMPRESSION
    virtual ~ZlibOutStream();

    void setUnderlying(OutStream* os);
    void flush();
    int length();

  private:

    int overrun(int itemSize, int nItems);

    OutStream* underlying;
    int bufSize;
    int offset;
    z_stream_s* zs;
    U8* start;
  };

} // end of namespace rdr

#endif
