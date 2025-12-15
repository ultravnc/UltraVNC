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


// This is an object and macros which provide general logging and debugging functions.
// It can log to a file, to a new console, and/or to debug - others maybe to follow.
// Every log object has a logging level (which can be changed).
// Only log requests with a high enough level attached get logged. So the
// level can be thought of as 'amount of detail'.
// We use Unicode-portable stuff here for compatibility with WinCE.
//
// Typical use:
//
//       Log log;
//       log.SetFile( _T("myapp.log") );
//       ...
//       log.Print(2, _T("x = %d\n"), x);
//

#ifndef VNCLOGGING
#define VNCLOGGING

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

class VNCLog
{
public:
	// Logging mode flags:
	static const int ToDebug;
	static const int ToFile;
	static const int ToConsole;

	// Create a new log object.
	// Parameters as follows:
	//    mode     - specifies where output should go, using combination
	//               of flags above.
	//    level    - the default level
	//    filename - if flag Log::ToFile is specified in the type,
	//               a filename must be specified here.
	//    append   - if logging to a file, whether or not to append to any
	//               existing log.
	VNCLog();
	void VNCLog::Print(int level, const char* format, ...);

	// Change the log level
	void SetLevel(int level);
	int  GetLevel() const { return m_level; };

	void SetVideo(bool enable) { m_video = enable; };
	bool GetVideo() { return m_video; };
	void SetPath(char path[512]);
	char* GetPath();
	void ClearAviConfig();

	// Change the logging mode
	void SetMode(int mode);
	int  GetMode() const { return m_mode; };

	// Change or set the logging filename. This only has an effect if
	// the log mode includes ToFile
	void SetFile();

	virtual ~VNCLog();

private:
	void ReallyPrintLine(const char* line);
	void ReallyPrint(const char* format, va_list ap);
	void ReallyPrintScreen(const char* format, va_list ap);
	void OpenFile();
	void CloseFile();
	bool m_tofile, m_todebug, m_toconsole;
	int m_mode;
	int m_level;
	HANDLE hlogfile;
	char m_filename[512];
	bool m_append;
	bool m_video;
	char m_path[MAX_PATH];

	time_t m_lastLogTime;
	void GetLastErrorMsg(LPSTR szErrorMsg) const;
};

#endif // VNCLOGGING
