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


#ifndef _WINVNC_ENCODEXZ
#define _WINVNC_ENCODEXZ

#include "vncEncoder.h"

namespace rdr { class xzOutStream; class MemOutStream; }

class vncEncodeXZ : public vncEncoder
{
public:
  vncEncodeXZ();
  ~vncEncodeXZ();

  virtual void Init();

  virtual UINT RequiredBuffSize(UINT width, UINT height);

  virtual UINT EncodeRect(BYTE *source, BYTE *dest, const rfb::Rect &rect);
  
  virtual UINT EncodeBulkRects(const rfb::RectVector &rects, BYTE *source, BYTE *dest, VSocket *outConn);

  void EncodeRect_Internal(BYTE *source, int x, int y, int w, int h);

  BOOL m_use_xzyw;

private:
  rdr::xzOutStream* xzos;
  rdr::MemOutStream* mos;
  void* beforeBuf;
};

#endif
