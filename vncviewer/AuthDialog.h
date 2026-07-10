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

enum DialogType{dtUserPass, dtPass, dtUserPassNotEncryption, dtPassUpgrade, dtUserPassRSA, dtPassRSA};

class AuthDialog  
{
public:
	AuthDialog();
	virtual ~AuthDialog();
	int DoDialog(DialogType dialogType, TCHAR IN_host[MAX_HOST_NAME_LEN], int IN_port, char hex[24] ="", char catchphrase[1024] ="");
	void SetStatusWindow(HWND hwndStatus, const wchar_t* className);
	char m_passwd[256];
	char m_domain[256];
	char m_user[256];
	static BOOL CALLBACK DlgProc(  HWND hwndDlg,  UINT uMsg, 
		WPARAM wParam, LPARAM lParam );
	static BOOL CALLBACK DlgProc1(  HWND hwndDlg,  UINT uMsg, 
		WPARAM wParam, LPARAM lParam );

	
	//adzm 2010-05-12 - passphrase
	bool m_bPassphraseMode;
	bool m_savePassword;
	wchar_t _host[MAX_HOST_NAME_LEN];
	int _port;
	DialogType dialogType;
	char hex[24]{};
	char catchphrase[1024]{};
	
	// For hiding status window during authentication when classname is used
	HWND m_hwndStatus;
	wchar_t m_className[255];
	bool m_statusWasVisible;
};
