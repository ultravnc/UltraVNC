// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


//need to be added for VS 2005
#define _Gii
#ifdef _Gii
#ifndef WINVER                  // Specifies that the minimum required platform is Windows 7.
#define WINVER 0x0602           // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows 7.
#define _WIN32_WINNT 0x0602     // Change this to the appropriate value to target other versions of Windows.
#endif     

//#ifndef _WIN32_WINDOWS          // Specifies that the minimum required platform is Windows 98.
//#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
//#endif

#ifndef _WIN32_IE                       // Specifies that the minimum required platform is Internet Explorer 7.0.
#define _WIN32_IE 0x0600        // Change this to the appropriate value to target other versions of IE.
#endif
#endif

#ifndef _HAS_STD_BYTE
#define _HAS_STD_BYTE 0
#endif

/*#ifdef _USE_DESKTOPDUPLICATION
#define _WIN32_WINNT 0x0602
#ifndef WINVER
#define WINVER 0x0602
#endif
#else
#define _WIN32_WINDOWS 0x0510
#ifndef WINVER
#define WINVER 0x0500
#endif
#endif*/

#define WIN32_LEAN_AND_MEAN
#ifndef STRICT
#define STRICT
#endif

//compile special case, rfb port is used for java and rfb
#include <ws2tcpip.h>
#include <winsock2.h>
#include <windows.h>
#include <shellapi.h>
//#include <winsock2.h>

#include <malloc.h>
#include <stdio.h>
#include <process.h>
#include <crtdbg.h>
#include "HelperFunctions.h"

//#include "dpi.h"

// LOGGING SUPPORT
void *memcpy_amd(void *dest, const void *src, size_t n);
bool CheckVideoDriver(bool);
#define MAXPATH 256

#include "vnclog.h"
extern VNCLog vnclog;

// No logging at all
#define LL_NONE		0
// Log server startup/shutdown
#define LL_STATE	0
// Log connect/disconnect
#define LL_CLIENTS	1
// Show on Logscreen
#define LL_LOGSCREEN -1
// Log connection errors (wrong pixfmt, etc)
#define LL_CONNERR	0
// Log socket errors
#define LL_SOCKERR	4
// Log internal errors
#define LL_INTERR	0

// Log internal warnings
#define LL_INTWARN	8
// Log internal info
#define LL_INTINFO	9
// Log socket errors
#define LL_SOCKINFO	10
// Log everything, including internal table setup, etc.
#define LL_ALL		10

// Macros for sticking in the current file name
#define VNCLOG(s)	(__FUNCTION__ " : " s)
//#if MSC_VER > 12
#ifndef _X64
#pragma comment(linker,"/manifestdependency:\"type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
//#endif
//#define memcpy memcpy_amd
//remove comment to compiler for >=athlon  or >=PIII

extern void WriteLog(char* sender, char *format, ...);
#ifdef _DEBUG
#define OutputDevMessage(...) WriteLog(__FUNCTION__, __VA_ARGS__);
#else
#define OutputDevMessage(...)
#endif

#ifdef HIGH_PRECISION
#define GetTimeFunction timeGetTime
#pragma comment(lib, "winmm.lib")
#else
#define GetTimeFunction GetTickCount
#endif
