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
// A FixedMemOutStream writes to a buffer of a fixed length.
//

#ifndef __RDR_FIXEDMEMOUTSTREAM_H__
#define __RDR_FIXEDMEMOUTSTREAM_H__

#include "OutStream.h
#include "Exception.h"

namespace rdr {

  class FixedMemOutStream : public OutStream {

  public:

    FixedMemOutStream(void* buf, int len) {
      ptr = start = (U8*)buf;
      end = start + len;
    }

    int length() { return ptr - start; }
    void reposition(int pos) { ptr = start + pos; }
    const void* data() { return (const void*)start; }

  private:

    int overrun(int itemSize, int nItems) { throw EndOfStream("overrun"); }
    U8* start;
  };

}

#endif
