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
// A MemOutStream grows as needed when data is written to it.
//

#ifndef __RDR_MEMOUTSTREAM_H__
#define __RDR_MEMOUTSTREAM_H__

#include "OutStream.h"

namespace rdr {

  class MemOutStream : public OutStream {

  public:

    MemOutStream(int len=1024) {
      start = ptr = new U8[len];
      end = start + len;
    }

    virtual ~MemOutStream() {
      delete [] start;
    }

    void writeBytes(const void* data, int length) {
      check(length);
      memcpy(ptr, data, length);
      ptr += length;
    }

    int length() { return (int)(ptr - start); }
    void clear() { ptr = start; };
    void reposition(int pos) { ptr = start + pos; }

    // data() returns a pointer to the buffer.

    const void* data() { return (const void*)start; }

  private:

    // overrun() either doubles the buffer or adds enough space for nItems of
    // size itemSize bytes.

    int overrun(int itemSize, int nItems) {
      int len = (int)(ptr - start + itemSize * nItems);
      if (len < (end - start) * 2)
        len = (int)((end - start) * 2);

      U8* newStart = new U8[len];
      memcpy(newStart, start, ptr - start);
      ptr = newStart + (ptr - start);
      delete [] start;
      start = newStart;
      end = newStart + len;

      return nItems;
    }

    U8* start;
  };

}

#endif
