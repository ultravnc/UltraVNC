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


#include "EventLogging.h"


EventLogging::EventLogging()
{
	// returns a handle that links the source to the registry 
	m_hEventLinker = RegisterEventSource(NULL,_T("UltraVNC"));

}

EventLogging::~EventLogging()
{
	// Releases the handle to the registry
	DeregisterEventSource(m_hEventLinker);
}



void EventLogging::LogIt(WORD CategoryID, DWORD EventID, LPCTSTR *ArrayOfStrings,
						 UINT NumOfArrayStr,LPVOID RawData,DWORD RawDataSize)
{

	// Writes data to the event log
	ReportEvent(m_hEventLinker,EVENTLOG_INFORMATION_TYPE,CategoryID,
		EventID,NULL,1,RawDataSize,ArrayOfStrings,RawData);	

}


void EventLogging::AddEventSourceToRegistry(LPCTSTR lpszSourceName)
{
    HKEY  hk;
    DWORD dwData;
    TCHAR szBuf[MAX_PATH];
    TCHAR szKey[255] =_T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
    TCHAR szServicePath[MAX_PATH];

    lstrcat(szKey, _T("UltraVNC"));

    if(RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hk) != ERROR_SUCCESS)
    {
        return;
    }

    if (GetModuleFileName(NULL, szServicePath, MAX_PATH))
		{
			TCHAR* p = _tcsrchr(szServicePath, '\\');
			if (p == NULL) return;
			*p = '\0';
			_tcscat_s(szServicePath,_T("\\logmessages.dll"));
		}
    lstrcpy(szBuf, szServicePath);

    // Add the name to the EventMessageFile subkey.
    if(RegSetValueEx(hk,
                     _T("EventMessageFile"),
                     0,
                     REG_EXPAND_SZ,
                     (LPBYTE) szBuf,
                     (lstrlen(szBuf) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS)
    {
        RegCloseKey(hk);
        return;
    }

    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |EVENTLOG_INFORMATION_TYPE;
    if(RegSetValueEx(hk,
                     _T("TypesSupported"),
                     0,
                     REG_DWORD,
                     (LPBYTE)&dwData,
                     sizeof(DWORD)) != ERROR_SUCCESS)
    {
        
    } RegCloseKey(hk);
}

void LOGV2(TCHAR *szMslogonLog, long EventID, const TCHAR *format, ...) {
    FILE *file;
	LPCTSTR ps[3];
	TCHAR textbuf[2 * MAXLEN] = _T("");
	char texttowrite[2 * MAXLEN] = "";
	TCHAR szTimestamp[MAXLEN] = _T("");
	TCHAR szText[MAXLEN] = _T("");
	SYSTEMTIME time;

	va_list ap;
	va_start(ap, format);
	_vstprintf_s(szText, format, ap);
	va_end(ap);
	// Prepend timestamp to message
	GetLocalTime(& time);
	_stprintf_s(szTimestamp,_T("%.2d/%.2d/%d %.2d:%.2d:%.2d\t"), 
		time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute, time.wSecond);
	_tcscpy_s(textbuf,szTimestamp);
	_tcscat_s(textbuf,szText);
	ps[0] = textbuf;
    EventLogging log;
	log.AddEventSourceToRegistry(NULL);
	log.LogIt(1,EventID, ps,1,NULL,0);
	file = _tfopen(szMslogonLog, _T("a"));
	if(file!=NULL) 
	{

		// Write ANSI
#if defined UNICODE || defined _UNICODE
		size_t pnconv;
		wcstombs_s(&pnconv, texttowrite, 512, textbuf, 2 * MAXLEN);
#else
		strcpy(texttowrite, texttowrite);
#endif
		fwrite(texttowrite, sizeof(char), strlen(texttowrite),file);
		fclose(file);
	}
}
 
void LOG(long EventID, const TCHAR* format, ...) {
	FILE* file;
	TCHAR szMslogonLog[MAX_PATH];
	LPCTSTR ps[3];
	TCHAR textbuf[2 * MAXLEN] = _T("");
	char texttowrite[2 * MAXLEN] = "";
	TCHAR szTimestamp[MAXLEN] = _T("");
	TCHAR szText[MAXLEN] = _T("");
	SYSTEMTIME time;

	va_list ap;
	va_start(ap, format);
	_vstprintf_s(szText, format, ap);
	va_end(ap);
	// Prepend timestamp to message
	GetLocalTime(&time);
	_stprintf_s(szTimestamp, _T("%.2d/%.2d/%d %.2d:%.2d:%.2d\t"),
		time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute, time.wSecond);
	_tcscpy_s(textbuf, szTimestamp);
	_tcscat_s(textbuf, szText);
	ps[0] = textbuf;
	EventLogging log;
	log.AddEventSourceToRegistry(NULL);
	log.LogIt(1, EventID, ps, 1, NULL, 0);
	if (GetModuleFileName(NULL, szMslogonLog, MAX_PATH))
	{
		TCHAR* p = _tcsrchr(szMslogonLog, '\\');
		if (p != NULL)
		{
			*p = '\0';
			_tcscat_s(szMslogonLog, _T("\\mslogon.log"));
		}
	}
	file = _tfopen(szMslogonLog, _T("a"));
	if (file != NULL)
	{

		// Write ANSI
#if defined UNICODE || defined _UNICODE
		size_t pnconv;
		wcstombs_s(&pnconv, texttowrite, 512, textbuf, 2 * MAXLEN);
#else
		strcpy(texttowrite, texttowrite);
#endif
		fwrite(texttowrite, sizeof(char), strlen(texttowrite), file);
		fclose(file);
	}
}