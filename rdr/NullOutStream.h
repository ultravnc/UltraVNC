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


#ifndef __RDR_NULLOUTSTREAM_H__
#define __RDR_NULLOUTSTREAM_H__

#include "OutStream.h"

namespace rdr {

  class NullOutStream : public OutStream {

  public:
    NullOutStream();
    virtual ~NullOutStream();
    int length();
    void writeBytes(const void* data, int length);

  private:
    int overrun(int itemSize, int nItems);
    int offset;
    U8* start;
  };

}

#endif
