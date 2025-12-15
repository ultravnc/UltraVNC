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
#define SECURITY_WIN32
#include <security.h>
#include <sspi.h>
#ifndef SEC_I_COMPLETE_NEEDED
#include <issperr.h>
#include <time.h>
#endif
#include <aclapi.h>
#pragma hdrstop

#include "Auth_Seq.h"

#define MAXLEN 256

BOOL CUPSD2(const char * domainuser, 
					   const char *password, 
					   PSECURITY_DESCRIPTOR psdSD,
					   PBOOL isAuthenticated,
					   PDWORD pdwAccessGranted);

BOOL WINAPI SSPLogonUser(LPTSTR szDomain, 
						 LPTSTR szUser, 
						 LPTSTR szPassword, 
						 PSECURITY_DESCRIPTOR psdSD, 
						 PBOOL isAuthenticated,
						 PDWORD pdwAccessGranted);

BOOL ImpersonateAndCheckAccess(PCtxtHandle phContext, 
							   PSECURITY_DESCRIPTOR psdSD, 
							   PDWORD pdwAccessGranted);

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

const char * SplitString(const char *input, char separator, char *head, int sizeHead);
bool IsImpersonationAllowed();
 
#define ViewOnly 0x0001
#define	Interact 0x0002
#define vncGenericRead (STANDARD_RIGHTS_READ | ViewOnly)
#define	vncGenericWrite	(STANDARD_RIGHTS_WRITE | Interact)
#define	vncGenericExecute (STANDARD_RIGHTS_EXECUTE)
#define	vncGenericAll (STANDARD_RIGHTS_ALL | ViewOnly | Interact)

void LOG(long EvenID, const TCHAR* format, ...);
void LOGV2(TCHAR* szMslogonLog, long EvenID, const TCHAR *format, ...);

bool QualifyName(const TCHAR *user, LPTSTR DomName);
bool isNT4();


