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


// vncAcceptDialog.cpp: implementation of the vncAcceptDialog class, used
// to query whether or not to accept incoming connections.

#include "stdhdrs.h"
#include "vncacceptdialog.h"
#include "winvnc.h"
#include "resource.h"
#include "common/win32_helpers.h"
#include "SettingsManager.h"

#include "Localization.h" // Act : add localization on messages

//	[v1.0.2-jp1 fix] Load resouce from dll
extern HINSTANCE	hInstResDLL;

// Constructor

vncAcceptDialog::vncAcceptDialog(UINT timeoutSecs,BOOL acceptOnTimeout, const char *ipAddress, char *infoMsg, bool notification)
{
	m_timeoutSecs = timeoutSecs;
	m_ipAddress = _strdup(ipAddress);
	m_foreground_hack=FALSE;
	m_acceptOnTimeout = acceptOnTimeout;
	ThreadHandle = NULL;
	this->infoMsg = infoMsg;
	this->notification = notification;
}

// Destructor

vncAcceptDialog::~vncAcceptDialog()
{
	if (m_ipAddress)
		free(m_ipAddress);
	if (ThreadHandle)
		CloseHandle(ThreadHandle);
}

// Routine called to activate the dialog and, once it's done, delete it
DWORD WINAPI DialogThread(LPVOID lpParam)
{
	vncAcceptDialog *dialog = (vncAcceptDialog*)lpParam;
	HDESK desktop;
	desktop = OpenInputDesktop(0, FALSE,
			DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
			DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
			DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
			DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());											
	SetThreadDesktop(desktop);

	//	[v1.0.2-jp1 fix]
	//int retVal = DialogBoxParam(hAppInstance, MAKEINTRESOURCE(IDD_ACCEPT_CONN),
	int retVal = DialogBoxParam(hInstResDLL, MAKEINTRESOURCE(IDD_ACCEPT_CONN),
		NULL, (DLGPROC) dialog->vncAcceptDlgProc, (LONG_PTR) dialog);

	SetThreadDesktop(old_desktop);
	CloseDesktop(desktop);
	switch (retVal)
	{
		case IDREJECT:
			return 0;
		case IDACCEPT:
			return 1;
	}
	return (dialog->m_acceptOnTimeout) ? 1 : 0;
}

BOOL vncAcceptDialog::DoDialog()
{
	DWORD dwTId; 
	ThreadHandle = CreateThread(NULL, 0, DialogThread, this, 0, &dwTId);	
	WaitForSingleObject(ThreadHandle, INFINITE);
	DWORD lpExitCode;
	GetExitCodeThread(ThreadHandle, &lpExitCode);
	if (lpExitCode == 1) return 1;
	return 0;
}

// Callback function - handles messages sent to the dialog box

BOOL CALLBACK vncAcceptDialog::vncAcceptDlgProc(HWND hwnd,
											UINT uMsg,
											WPARAM wParam,
											LPARAM lParam) {
	// This is a static method, so we don't know which instantiation we're
	// dealing with. But we can get a pseudo-this from the parameter to
	// WM_INITDIALOG, which we therafter store with the window and retrieve
	// as follows:
	 static HBITMAP hbmBkGnd = NULL;
     vncAcceptDialog *_this = helper::SafeGetWindowUserData<vncAcceptDialog>(hwnd);
	switch (uMsg) {
		// Dialog has just been created
	case WM_INITDIALOG:
		{
			HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_WINVNC));
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			// Save the lParam into our user data so that subsequent calls have
			// access to the parent C++ object
            helper::SafeSetWindowUserData(hwnd, lParam);
            vncAcceptDialog *_this = (vncAcceptDialog *) lParam;

			// Set the IP-address string
			char accept_reject_mesg[512];
			strcpy_s(accept_reject_mesg, settings->getAccept_reject_mesg());

			if (strlen(accept_reject_mesg) == 0) 
				strcpy_s(accept_reject_mesg,"UltraVNC Server has received an incoming connection from");

			if (strlen(_this->infoMsg) > 0) {
				strcat_s(accept_reject_mesg, "\r\n");
				strcat_s(accept_reject_mesg, _this->infoMsg);
			}
			SetDlgItemText(hwnd, IDC_STATIC_TEXT1, accept_reject_mesg);
			SetDlgItemText(hwnd, IDC_ACCEPT_IP, _this->m_ipAddress);

			{
			char mycommand[MAX_PATH];	
			strcpy_s(mycommand, winvncFolder);
			strcat_s(mycommand,"\\mylogo.bmp");
			hbmBkGnd = (HBITMAP)LoadImage(NULL, mycommand, IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
			}
			SendMessage(GetDlgItem(hwnd, IDC_ACCEPTLOGO),STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)(HBITMAP)hbmBkGnd);

			if (SetTimer(hwnd, 1, 1000, NULL) == 0)
			{
				if (_this->m_acceptOnTimeout)
					EndDialog(hwnd, IDACCEPT);
				else
				EndDialog(hwnd, IDREJECT);
			}
			_this->m_timeoutCount = _this->m_timeoutSecs;

			char temp[256];
			if (_this->m_acceptOnTimeout)
				sprintf_s(temp, "AutoAccept:%u", (_this->m_timeoutCount));
			else
				sprintf_s(temp, "AutoReject:%u", (_this->m_timeoutCount));
			SetDlgItemText(hwnd, IDC_ACCEPT_TIMEOUT, temp);

			if (!_this->m_foreground_hack) {
				SetForegroundWindow(hwnd);
			}

			// Beep
			MessageBeep(MB_ICONEXCLAMATION);

            // Return success!
			return TRUE;
		}

		// Timer event
	case WM_TIMER:
		if ((_this->m_timeoutCount) == 0)
			{
				if ( _this->m_acceptOnTimeout )
					{
						EndDialog(hwnd, IDACCEPT);
					}
				else
					{
						EndDialog(hwnd, IDREJECT);
					}
			}
		_this->m_timeoutCount--;

		// Flash if necessary
		if (_this->m_foreground_hack) {
			if (GetWindowThreadProcessId(GetForegroundWindow(), NULL) != GetCurrentProcessId())
			{
				_this->m_flash_state = !_this->m_flash_state;
				FlashWindow(hwnd, _this->m_flash_state);
			} else {
				_this->m_foreground_hack = FALSE;
			}
		}

		// Update the displayed count
		char temp[256];
		if ( _this->m_acceptOnTimeout )
			sprintf_s(temp, "AutoAccept: %u", (_this->m_timeoutCount));
		else
			sprintf_s(temp, "AutoReject: %u", (_this->m_timeoutCount));
		SetDlgItemText(hwnd, IDC_ACCEPT_TIMEOUT, temp);
		break;

		// Dialog has just received a command
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			// User clicked Accept or pressed return
		case IDACCEPT:
		case IDOK:
			EndDialog(hwnd, IDACCEPT);
			return TRUE;

		case IDREJECT:
		case IDCANCEL:
			EndDialog(hwnd, IDREJECT);
			return TRUE;
		};

		break;

		// Window is being destroyed!  (Should never happen)
	case WM_DESTROY:
		DeleteObject(hbmBkGnd);
		EndDialog(hwnd, IDREJECT);
		return TRUE;
	}
	return 0;
}