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


class vncAcceptDialog;

#if (!defined(_WINVNC_VNCACCEPTDIALOG))
#define _WINVNC_VNCACCEPTDIALOG

#pragma once

// Incoming connection-rejection dialog. vncClient creates an AcceptDialog
// if it needs to query whether or not to accept a connection.

class vncAcceptDialog  
{
public:

	vncAcceptDialog(UINT timeoutSecs,BOOL acceptOnTimeout, const char *ipAddress, char* infoMsg, bool notification);
	virtual ~vncAcceptDialog();
	BOOL DoDialog();
	BOOL m_acceptOnTimeout;
	static BOOL CALLBACK vncAcceptDlgProc(HWND hwndDlg,UINT uMsg, WPARAM wParam,LPARAM lParam);
private:
	// Storage for the timeout value
	UINT m_timeoutSecs;
	UINT m_timeoutCount;

	// Flashing hack
	BOOL m_foreground_hack;
	BOOL m_flash_state;

	// Address of the offending machine
	char *m_ipAddress;	
	HANDLE ThreadHandle;
	char *infoMsg;
	bool notification;
};

#endif
