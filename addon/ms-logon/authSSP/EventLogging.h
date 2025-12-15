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


#define _CRT_NON_CONFORMING_SWPRINTFS 1

#include <windows.h>
#include <stdio.h>
#include <lmcons.h>
#include <stdlib.h>
#include <wchar.h>
#include <tchar.h>
#include <lm.h>
#include <stdio.h>
#define SECURITY_WIN32
#include <sspi.h>
#ifndef SEC_I_COMPLETE_NEEDED
#include <issperr.h>
#include <time.h>
#endif
#pragma hdrstop
#include <stdarg.h>

#define MAXLEN 256
#define MAX_PREFERRED_LENGTH    ((DWORD) -1)
#define BUFSIZE 1024

class EventLogging
{
public:
	EventLogging();
	virtual ~EventLogging();
	virtual void LogIt(WORD CategoryID, DWORD EventID, LPCTSTR *ArrayOfStrings,
		UINT NumOfArrayStr = 0,LPVOID RawData = NULL,DWORD RawDataSize = 0);
	HANDLE m_hEventLinker;
	void AddEventSourceToRegistry(LPCTSTR lpszSourceName);

};

void LOGV2(TCHAR* szMslogonLog, long EvenID, const TCHAR *format, ...);
void LOG(long EvenID, const TCHAR* format, ...);