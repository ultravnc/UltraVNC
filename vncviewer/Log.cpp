// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


// Log.cpp: implementation of the Log class.
//
//////////////////////////////////////////////////////////////////////

#include "stdhdrs.h"
#include "Log.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const int Log::ToDebug   =  1;
const int Log::ToFile    =  2;
const int Log::ToConsole =  4;

const static int LINE_BUFFER_SIZE = 1024;

Log::Log(int mode, int level, LPTSTR filename, bool append)
{
    hlogfile = NULL;
    m_todebug = false;
    m_toconsole = false;
    m_tofile = false;
    SetMode(mode);
    if (mode & ToFile)  {
        SetFile(filename, append);
    }
}

void Log::SetMode(int mode) {
    
    if (mode & ToDebug)
        m_todebug = true;
    else
        m_todebug = false;

    if (mode & ToFile)  {
        m_tofile = true;
    } else {
        CloseFile();
        m_tofile = false;
    }
    
    if (mode & ToConsole) {
        if (!m_toconsole) {
            AllocConsole();
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
        }

        m_toconsole = true;
    } else {
        m_toconsole = false;
    }
}


void Log::SetLevel(int level) {
    m_level = level;
}

void Log::SetFile(LPTSTR filename, bool append) 
{
    // if a log file is open, close it now.
    CloseFile();

    m_tofile  = true;
    
    // If filename is NULL or invalid we should throw an exception here
    
    hlogfile = CreateFile(
        filename,  GENERIC_WRITE, FILE_SHARE_READ, NULL,
        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL  );
    
    if (hlogfile == INVALID_HANDLE_VALUE) {
        // We should throw an exception here
        m_todebug = true;
        m_tofile = false;
        Print(0, _T("Error opening log file %s\n"), filename);
    }
    if (append) {
        SetFilePointer( hlogfile, 0, NULL, FILE_END );
    } else {
        SetEndOfFile( hlogfile );
    }
}

// if a log file is open, close it now.
void Log::CloseFile() {
    if (hlogfile != NULL) {
        CloseHandle(hlogfile);
        hlogfile = NULL;
    }
}

void Log::ReallyPrint(LPCTSTR format, va_list ap)
{
    WCHAR line[LINE_BUFFER_SIZE];
    _vsnwprintf_s(line, LINE_BUFFER_SIZE, _TRUNCATE, format, ap);
    if (m_todebug) OutputDebugStringW(line);

    if (m_toconsole) {
        DWORD charsWritten;
        WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), line, (DWORD)wcslen(line), &charsWritten, NULL);
    };

    if (m_tofile && (hlogfile != NULL)) {
        char lineA[LINE_BUFFER_SIZE];
        WideCharToMultiByte(CP_UTF8, 0, line, -1, lineA, LINE_BUFFER_SIZE, NULL, NULL);
        DWORD byteswritten;
        WriteFile(hlogfile, lineA, (DWORD)strlen(lineA), &byteswritten, NULL);
    }
}

Log::~Log()
{
    CloseFile();
}

Log theLog;
