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
// ZlibInStream streams from a compressed data stream ("underlying"),
// decompressing with zlib on the fly.
//

#ifndef __RDR_ZLIBINSTREAM_H__
#define __RDR_ZLIBINSTREAM_H__

#pragma once

#include "InStream.h"

struct z_stream_s;

namespace rdr {

  class ZlibInStream : public InStream {

  public:

    ZlibInStream(int bufSize=0);
    virtual ~ZlibInStream();

    void setUnderlying(InStream* is, int bytesIn);
    void reset();
    int pos();

  private:

    int overrun(int itemSize, int nItems);
    void decompress();

    InStream* underlying;
    int bufSize;
    int offset;
    z_stream_s* zs;
    int bytesIn;
    U8* start;
  };

} // end of namespace rdr

#endif
