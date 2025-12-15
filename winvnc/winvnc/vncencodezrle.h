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


#ifndef _WINVNC_ENCODEZRLE
#define _WINVNC_ENCODEZRLE

#include "vncencoder.h"

namespace rdr { class ZlibOutStream; class MemOutStream; class ZstdOutStream; }
class vncEncodeZRLE : public vncEncoder
{
public:
  vncEncodeZRLE();
  ~vncEncodeZRLE();

  virtual void Init();

  virtual UINT RequiredBuffSize(UINT width, UINT height);

  virtual UINT EncodeRect(BYTE *source, BYTE *dest, const rfb::Rect &rect);

  BOOL m_use_zywrle;

private:
  rdr::ZlibOutStream* zos;
  rdr::ZstdOutStream* zstdos;
  rdr::MemOutStream* mos;
  void* beforeBuf;
};

#endif
