/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 TightVNC. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


// vncPropertiesPoll.cpp

// Implementation of the Properties dialog!

#include "stdhdrs.h"
#include "lmcons.h"
#include "winvnc.h"
#include "vncpropertiesPoll.h"
#include "vncserver.h"
#include "vncOSVersion.h"
#include <shlobj.h>
#include "common/win32_helpers.h"
#include "Localization.h" // ACT : Add localization on messages
#include "SettingsManager.h"
#include "credentials.h"
// [v1.0.2-jp1 fix] Load resouce from dll
extern HINSTANCE	hInstResDLL;

// Constructor & Destructor
vncPropertiesPoll::vncPropertiesPoll()
{
	m_dlgvisible = FALSE;
}

vncPropertiesPoll::~vncPropertiesPoll()
{
}

// Initialisation
BOOL
vncPropertiesPoll::Init(vncServer* server)
{
	// Save the server pointer
	m_server = server;
	LoadFromIniFile();
	return TRUE;
}

// Dialog box handling functions
void
vncPropertiesPoll::Show()
{
	DesktopUsersToken desktopUsersToken;
	HANDLE hPToken = desktopUsersToken.getDesktopUsersToken();
	int iImpersonateResult = 0;
	if (hPToken) {
		if (!ImpersonateLoggedOnUser(hPToken))
			iImpersonateResult = GetLastError();
	}

	// Now, if the dialog is not already displayed, show it!
	if (!m_dlgvisible) {
		for (;;) {
			int result = (int)DialogBoxParam(hInstResDLL, MAKEINTRESOURCE(IDD_PROPERTIES), NULL,
				(DLGPROC)DialogProcPoll, (LONG_PTR)this);
			vnclog.Print(LL_INTINFO, VNCLOG("dialog result = %d\n"), result);
			if (result == -1) {
				// Dialog box failed, so quit
				PostQuitMessage(0);
				if (iImpersonateResult == ERROR_SUCCESS)
					RevertToSelf();
				return;
			}
			break;
			omni_thread::sleep(4);
		}
	}

	if (iImpersonateResult == ERROR_SUCCESS)
		RevertToSelf();
}

BOOL CALLBACK vncPropertiesPoll::DialogProcPoll(HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	// We use the dialog-box's USERDATA to store a _this pointer
	// This is set only once WM_INITDIALOG has been recieved, though!
	vncPropertiesPoll* _this = helper::SafeGetWindowUserData<vncPropertiesPoll>(hwnd);

	switch (uMsg) {
	case WM_HSCROLL: {
		if ((lParam != 0) && (reinterpret_cast<HWND>(lParam) == GetDlgItem(hwnd, IDC_SLIDERFPS))) {
			switch (LOWORD(wParam)) {
			case SB_ENDSCROLL:
			case SB_LEFT:
			case SB_RIGHT:
			case SB_LINELEFT:
			case SB_LINERIGHT:
			case SB_PAGELEFT:
			case SB_PAGERIGHT:
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				int pos = SendMessage(GetDlgItem(hwnd, IDC_SLIDERFPS), TBM_GETPOS, 0, 0L);
				CHAR temp[250];
				sprintf_s(temp, "%d", pos);
				SetDlgItemText(hwnd, IDC_STATICFPS, temp);
				settings->setMaxFPS(pos);
				break;
			}
		}
		break;
	}
				   return (INT_PTR)FALSE;

	case WM_INITDIALOG:
	{
		// Retrieve the Dialog box parameter and use it as a pointer
		// to the calling vncPropertiesPoll object
		helper::SafeSetWindowUserData(hwnd, lParam);
		_this = (vncPropertiesPoll*)lParam;
		_this->m_dlgvisible = TRUE;

		HWND slider = GetDlgItem(hwnd, IDC_SLIDERFPS);
		SendMessage(slider, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(5, 60));
		SendMessage(slider, TBM_SETTICFREQ, 1, 10);
		SendMessage(slider, TBM_SETPOS, true, settings->getMaxFPS());
		CHAR temp[250];
		sprintf_s(temp, "%d", settings->getMaxFPS());
		SetDlgItemText(hwnd, IDC_STATICFPS, temp);

		if (settings->getddEngine()) {
			ShowWindow(GetDlgItem(hwnd, IDC_CHECKDRIVER), false);
			ShowWindow(GetDlgItem(hwnd, IDC_STATICELEVATED), false);
			EnableWindow(GetDlgItem(hwnd, IDC_SLIDERFPS), settings->getDriver());
			EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS60), settings->getDriver());
			EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS5), settings->getDriver());
			SetWindowText(GetDlgItem(hwnd, IDC_DRIVER), "DDEngine (restart on change required)");
		}
		else {
			EnableWindow(GetDlgItem(hwnd, IDC_SLIDERFPS), false);
			EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS60), false);
			EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS5), false);
		}

		// Modif sf@2002
		SetDlgItemInt(hwnd, IDC_MAXCPU, settings->getMaxCpu(), false);

		HWND hTurboMode = GetDlgItem(hwnd, IDC_TURBOMODE);
		SendMessage(hTurboMode, BM_SETCHECK, settings->getTurboMode(), 0);

		// Set the polling options
		HWND hDriver = GetDlgItem(hwnd, IDC_DRIVER);
		SendMessage(hDriver,
			BM_SETCHECK,
			settings->getDriver(), 0);

		HWND hHook = GetDlgItem(hwnd, IDC_HOOK);
		SendMessage(hHook,
			BM_SETCHECK,
			settings->getHook(),
			0);

		HWND hPollFullScreen = GetDlgItem(hwnd, IDC_POLL_FULLSCREEN);
		SendMessage(hPollFullScreen,
			BM_SETCHECK,
			settings->getPollFullScreen(),
			0);

		HWND hPollForeground = GetDlgItem(hwnd, IDC_POLL_FOREGROUND);
		SendMessage(hPollForeground,
			BM_SETCHECK,
			settings->getPollForeground(),
			0);

		HWND hPollUnderCursor = GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR);
		SendMessage(hPollUnderCursor,
			BM_SETCHECK,
			settings->getPollUnderCursor(),
			0);

		HWND hPollConsoleOnly = GetDlgItem(hwnd, IDC_CONSOLE_ONLY);
		SendMessage(hPollConsoleOnly,
			BM_SETCHECK,
			settings->getPollConsoleOnly(),
			0);
		EnableWindow(hPollConsoleOnly,
			settings->getPollUnderCursor() || settings->getPollForeground()
		);

		HWND hPollOnEventOnly = GetDlgItem(hwnd, IDC_ONEVENT_ONLY);
		SendMessage(hPollOnEventOnly,
			BM_SETCHECK,
			settings->getPollOnEventOnly(),
			0);
		EnableWindow(hPollOnEventOnly,
			settings->getPollUnderCursor() || settings->getPollForeground()
		);

		// [<--v1.0.2-jp2 fix]

		CheckDlgButton(hwnd, IDC_AUTOCAPT1,
			(settings->getAutocapt() == 1) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, IDC_AUTOCAPT2,
			(settings->getAutocapt() == 2) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, IDC_AUTOCAPT3,
			(settings->getAutocapt() == 3) ? BST_CHECKED : BST_UNCHECKED);

		SetForegroundWindow(hwnd);

		return FALSE; // Because we've set the focus
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_DRIVER:
			if (SendMessage(GetDlgItem(hwnd, IDC_DRIVER), BM_GETCHECK, 0, 0) == BST_CHECKED)
				_this->m_server->Driver(CheckVideoDriver(0));
			else
				_this->m_server->Driver(false);
			if (settings->getddEngine()) {
				EnableWindow(GetDlgItem(hwnd, IDC_SLIDERFPS), settings->getDriver());
				EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS60), settings->getDriver());
				EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS5), settings->getDriver());
			}
			break;
		case IDOK:
		case IDC_APPLY:
		{
			int maxcpu = GetDlgItemInt(hwnd, IDC_MAXCPU, NULL, FALSE);
			settings->setMaxCpu(maxcpu);

			int pos = SendMessage(GetDlgItem(hwnd, IDC_SLIDERFPS), TBM_GETPOS, 0, 0L);
			settings->setMaxFPS(pos);

			HWND hcapt = GetDlgItem(hwnd, IDC_AUTOCAPT1);
			if (SendMessage(hcapt, BM_GETCHECK, 0, 0) == BST_CHECKED)
				settings->setAutocapt(1);
			hcapt = GetDlgItem(hwnd, IDC_AUTOCAPT2);
			if (SendMessage(hcapt, BM_GETCHECK, 0, 0) == BST_CHECKED)
				settings->setAutocapt(2);
			hcapt = GetDlgItem(hwnd, IDC_AUTOCAPT3);
			if (SendMessage(hcapt, BM_GETCHECK, 0, 0) == BST_CHECKED)
				settings->setAutocapt(3);

			settings->setAutocapt(settings->getAutocapt());
			// Modif sf@2002
			HWND hTurboMode = GetDlgItem(hwnd, IDC_TURBOMODE);
			settings->setTurboMode(SendMessage(hTurboMode, BM_GETCHECK, 0, 0) == BST_CHECKED);

			// Handle the polling stuff
			HWND hDriver = GetDlgItem(hwnd, IDC_DRIVER);
			bool result = (SendMessage(hDriver, BM_GETCHECK, 0, 0) == BST_CHECKED);
			if (result)
			{
				_this->m_server->Driver(CheckVideoDriver(0));
			}
			else _this->m_server->Driver(false);
			HWND hHook = GetDlgItem(hwnd, IDC_HOOK);
			_this->m_server->Hook(
				SendMessage(hHook, BM_GETCHECK, 0, 0) == BST_CHECKED
			);
			HWND hPollFullScreen = GetDlgItem(hwnd, IDC_POLL_FULLSCREEN);
			settings->setPollFullScreen(
				SendMessage(hPollFullScreen, BM_GETCHECK, 0, 0) == BST_CHECKED
			);

			HWND hPollForeground = GetDlgItem(hwnd, IDC_POLL_FOREGROUND);
			settings->setPollForeground(
				SendMessage(hPollForeground, BM_GETCHECK, 0, 0) == BST_CHECKED
			);

			HWND hPollUnderCursor = GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR);
			settings->setPollUnderCursor(
				SendMessage(hPollUnderCursor, BM_GETCHECK, 0, 0) == BST_CHECKED
			);

			HWND hPollConsoleOnly = GetDlgItem(hwnd, IDC_CONSOLE_ONLY);
			settings->setPollConsoleOnly(
				SendMessage(hPollConsoleOnly, BM_GETCHECK, 0, 0) == BST_CHECKED
			);

			HWND hPollOnEventOnly = GetDlgItem(hwnd, IDC_ONEVENT_ONLY);
			settings->setPollOnEventOnly(
				SendMessage(hPollOnEventOnly, BM_GETCHECK, 0, 0) == BST_CHECKED
			);
#ifndef SC_20
			_this->SaveToIniFile();
#endif
			// Was ok pressed?
			if (LOWORD(wParam) == IDOK)
			{
				// Yes, so close the dialog
				vnclog.Print(LL_INTINFO, VNCLOG("enddialog (OK)\n"));

				EndDialog(hwnd, IDOK);
				_this->m_dlgvisible = FALSE;
			}

			_this->m_server->SetHookings();

			return TRUE;
		}

		case IDCANCEL:
			vnclog.Print(LL_INTINFO, VNCLOG("enddialog (CANCEL)\n"));

			EndDialog(hwnd, IDCANCEL);
			_this->m_dlgvisible = FALSE;
			return TRUE;

		case IDC_POLL_FOREGROUND:
		case IDC_POLL_UNDER_CURSOR:
			// User has clicked on one of the polling mode buttons
			// affected by the pollconsole and pollonevent options
		{
			// Get the poll-mode buttons
			HWND hPollForeground = GetDlgItem(hwnd, IDC_POLL_FOREGROUND);
			HWND hPollUnderCursor = GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR);

			// Determine whether to enable the modifier options
			BOOL enabled = (SendMessage(hPollForeground, BM_GETCHECK, 0, 0) == BST_CHECKED) ||
				(SendMessage(hPollUnderCursor, BM_GETCHECK, 0, 0) == BST_CHECKED);

			HWND hPollConsoleOnly = GetDlgItem(hwnd, IDC_CONSOLE_ONLY);
			EnableWindow(hPollConsoleOnly, enabled);

			HWND hPollOnEventOnly = GetDlgItem(hwnd, IDC_ONEVENT_ONLY);
			EnableWindow(hPollOnEventOnly, enabled);
		}
		return TRUE;

		case IDC_CHECKDRIVER:
		{
			CheckVideoDriver(1);
		}
		return TRUE;
		}
		break;
	}
	return 0;
}

void vncPropertiesPoll::LoadFromIniFile()
{
#ifndef SC_20
	settings->load();
#endif
	if (!(CheckVideoDriver(0) && settings->getDriver()))
		settings->setDriver(false);
}

void vncPropertiesPoll::SaveToIniFile()
{
#ifndef SC_20
	settings->save();
#endif
}

