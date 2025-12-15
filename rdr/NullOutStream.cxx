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


#include "NullOutStream.h"
#include "Exception.h"

using namespace rdr;

static const int bufferSize = 1024;

NullOutStream::NullOutStream()
  : offset(0)
{
  start = ptr = new U8[bufferSize];
  end = start + bufferSize;
}

NullOutStream::~NullOutStream()
{
  delete [] start;
}

int NullOutStream::length()
{
  return (int)(offset + ptr - start);
}

void NullOutStream::writeBytes(const void* data, int length)
{
  offset += length;
}

int NullOutStream::overrun(int itemSize, int nItems)
{
  if (itemSize > bufferSize)
    throw Exception("NullOutStream overrun: max itemSize exceeded");

  offset += (int)(ptr - start);
  ptr = start;

  if (itemSize * nItems > end - ptr)
    nItems = (int)((end - ptr) / itemSize);

  return nItems;
}
