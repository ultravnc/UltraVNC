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


#include "InStream.h"
#include "Exception.h"

using namespace rdr;

U32 InStream::maxStringLength = 65535;

char* InStream::readString()
{
  U32 len = readU32();
  if (len > maxStringLength)
    throw Exception("InStream max string length exceeded");
  char* str = new char[len+1];
  readBytes(str, len);
  str[len] = 0;
  return str;
}
