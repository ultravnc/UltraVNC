// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2005 Sean E. Covel All Rights Reserved.
//


#ifndef _ENVREG_H
#define _ENVREG_H

#include "registry.h"
#include <tchar.h>
#include <stdlib.h>


static TCHAR * PROGRAMFILES = _T("programfiles");
static TCHAR * PROGRAMFILESDIR = _T("programfilesdir");

BOOL GetEnvironmentVariableFromRegistry(LPTSTR lpName, LPTSTR buffer, DWORD nSize);

#endif


