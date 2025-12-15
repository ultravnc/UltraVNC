// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2004 Martin Scharpf. All Rights Reserved.
//


// /macine-vnc Greg Wood (wood@agressiv.com)


// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the AUTHSSP_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// AUTHSSP_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef AUTHSSP_EXPORTS
#define AUTHSSP_API __declspec(dllexport)
#else
#define AUTHSSP_API __declspec(dllimport)
#endif

#include <tchar.h>
#include "vncAccessControl.h"

typedef BOOL (*CheckUserPasswordSDFn)(const char * domainuser,
									  const char *password,
									  PSECURITY_DESCRIPTOR psdSD,
									  PBOOL isAuthenticated,
									  PDWORD pdwAccessGranted);

#define MAXSTRING 254

AUTHSSP_API int CUPSD(const char* userin, const char* password, const char* machine);
void LOG(long EvenID, const TCHAR* format, ...);
AUTHSSP_API int CUPSDV2(const char * userin, const char *password, const char *machine, TCHAR* szMslogonLog);
void LOGV2(TCHAR* szMslogonLog, long EvenID, const TCHAR *format, ...);
TCHAR * AddToModuleDir(TCHAR *filename, int length);

extern BOOL CUPSD2(const char*userin, const char *password, PSECURITY_DESCRIPTOR psdSD, PBOOL pisAuthenticated, PDWORD pdwAccessGranted);