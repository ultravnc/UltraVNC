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


// vncSetAuth

// Object implementing the About dialog for UltraVNC Server.
#include "inifile.h"
#define MAXPATH 256
class vncSetAuth;

#if (!defined(_WINVNC_VNCSETAUTH))
#define _WINVNC_VNCSETAUTH

// The vncSetAuth class itself
class vncSetAuth
{
public:
	// Constructor/destructor
	vncSetAuth();
	~vncSetAuth();
	BOOL Init();

	// The dialog box window proc
	static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// General
	void Show(BOOL show);
	void SetTemp(char *TempFile){strcpy(m_Tempfile,TempFile);};
	char m_Tempfile[MAXPATH];

	// Implementation
	BOOL m_dlgvisible;

	LONG LoadInt_(LPCSTR valname, LONG defval);
	TCHAR * LoadString_(LPCSTR keyname);
	void SaveInt_(LPCSTR valname, LONG val);
	void SaveString_(LPCSTR valname, TCHAR *buffer);
	void savegroup1(TCHAR *value);
	TCHAR* Readgroup1();
	void savegroup2(TCHAR *value);
	TCHAR* Readgroup2();
	void savegroup3(TCHAR *value);
	TCHAR* Readgroup3();
	LONG Readlocdom1(LONG returnvalue);
	void savelocdom1(LONG value);
	LONG Readlocdom2(LONG returnvalue);
	void savelocdom2(LONG value);
	LONG Readlocdom3(LONG returnvalue);
	void savelocdom3(LONG value);

	char *group1;
	char *group2;
	char *group3;
	long locdom1;
	long locdom2;
	long locdom3;
	char pszgroup1[256];
	char pszgroup2[256];
	char pszgroup3[256];

	IniFile myIniFile;
};

#endif // _WINVNC_VNCPROPERTIES
