// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#pragma once

// Exceptions used in UltraVNC Viewer

class Exception  
{
public:
	Exception(const char *info, int error_nr = 0);
	virtual void Report() = 0;
	virtual ~Exception();
	char *m_info;
	int m_error_nr;
};

// This indicates something that the catcher should close 
// the connection quietly.
// Report() will display a TRACE message
class QuietException : public Exception {
public:
	QuietException(const char *info, int error_nr = 0);
	virtual void Report();
	virtual ~QuietException();
};

// This indicates something the user should be told about.
// In situations of ambiguity, the 'close' parameter can be used
// to specify whether or not the connection is closed as a result.
// In general it will be.
// Report() will display a message box
class WarningException : public Exception {
public:
	WarningException(const char *info, int error_nr = 0, bool close = true);
	virtual void Report();
	virtual ~WarningException();
	bool m_close;
};

// This is serious stuff - similar to an assert - we may not use?
// Report will display an important message box. Connection definitely
// closed.
class ErrorException : public Exception {
public:
	ErrorException(const char *info, int error_nr = 0);
	virtual void Report();
	virtual ~ErrorException();
};
