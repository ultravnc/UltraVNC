// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include "ScreenCapture.h"

//just make it run with SDK 8.1
#ifndef _WIN32_WINNT_WINTHRESHOLD
#define _WIN32_WINNT_WINTHRESHOLD           0x0A00 
VERSIONHELPERAPI
IsWindows10OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINTHRESHOLD), LOBYTE(_WIN32_WINNT_WINTHRESHOLD), 0);
}
#endif

//----------------------------------------------------------
ScreenCapture::ScreenCapture()
{
}
//----------------------------------------------------------
int ScreenCapture::osVersion()
{
	int version = OSOLD;
	if (IsWindowsXPSP3OrGreater())
		version = OSWINXP;
	if (IsWindowsVistaOrGreater())
		version = OSVISTA;
	if (IsWindows8OrGreater())
		version = OSWIN10;
	return version;
}
