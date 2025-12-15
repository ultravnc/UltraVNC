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


#include <windows.h>
#include <stdio.h>
#include <lmcons.h>
#include <stdlib.h>
#include <wchar.h>
#include <tchar.h>
#include <lm.h>
#include <stdio.h>
#define SECURITY_WIN32
#include <security.h>
#include <sspi.h>
#ifndef SEC_I_COMPLETE_NEEDED
#include <issperr.h>
#include <time.h>
#endif
#include <aclapi.h>
#pragma hdrstop

#define MAXLEN 256
#define MAX_PREFERRED_LENGTH    ((DWORD) -1)
#define BUFSIZE 1024

typedef DWORD (__stdcall *NetApiBufferFree_t)( void *buf );

typedef DWORD (__stdcall *NetWkstaGetInfoNT_t)( wchar_t *server, DWORD level, byte **buf );
typedef DWORD (__stdcall *NetWkstaGetInfo95_t)( char *domain,DWORD level, byte **buf );

#include "Auth_Seq.h"

BOOL GenClientContext(PAUTH_SEQ pAS, 
					  PSEC_WINNT_AUTH_IDENTITY pAuthIdentity,
					  PVOID pIn, 
					  DWORD cbIn, 
					  PVOID pOut, 
					  PDWORD pcbOut, 
					  PBOOL pfDone);

BOOL GenServerContext(PAUTH_SEQ pAS, 
					  PVOID pIn, 
					  DWORD cbIn, 
					  PVOID pOut,
					  PDWORD pcbOut, 
					  PBOOL pfDone);

void UnloadSecurityDll(HMODULE hModule);

HMODULE LoadSecurityDll();
