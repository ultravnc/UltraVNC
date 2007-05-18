//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 TightVNC. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
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
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// vncPropertiesPoll.cpp

// Implementation of the Properties dialog!

#include "stdhdrs.h"
#include "lmcons.h"
#include "vncService.h"

#include "WinVNC.h"
#include "vncPropertiesPoll.h"
#include "vncServer.h"
#include "vncOSVersion.h"


#include "localization.h" // ACT : Add localization on messages

const char WINVNC_REGISTRY_KEY [] = "Software\\ORL\\WinVNC3";

// Constructor & Destructor
vncPropertiesPoll::vncPropertiesPoll()
{
	m_dlgvisible = FALSE;
	m_usersettings = TRUE;
}

vncPropertiesPoll::~vncPropertiesPoll()
{
}

// Initialisation
BOOL
vncPropertiesPoll::Init(vncServer *server)
{
	// Save the server pointer
	m_server = server;
#ifndef SINGLECLICKULTRA_TEST	
	// Load the settings from the registry
	Load(TRUE);
#endif
	return TRUE;
}
#ifndef SINGLECLICKULTRA_TEST
// Dialog box handling functions
void
vncPropertiesPoll::Show(BOOL show, BOOL usersettings)
{
	if (show)
	{

		// Verify that we know who is logged on
		if (usersettings) {
			char username[UNLEN+1];
			if (!vncService::CurrentUser(username, sizeof(username)))
				return;
			if (strcmp(username, "") == 0) {
				MessageBox(NULL, sz_ID_NO_CURRENT_USER_ERR, sz_ID_WINVNC_ERROR, MB_OK | MB_ICONEXCLAMATION);
				return;
			}
		} else {
			// We're trying to edit the default local settings - verify that we can
			HKEY hkLocal, hkDefault;
			BOOL canEditDefaultPrefs = 1;
			DWORD dw;
			if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
				WINVNC_REGISTRY_KEY,
				0, REG_NONE, REG_OPTION_NON_VOLATILE,
				KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
				canEditDefaultPrefs = 0;
			else if (RegCreateKeyEx(hkLocal,
				"Default",
				0, REG_NONE, REG_OPTION_NON_VOLATILE,
				KEY_WRITE | KEY_READ, NULL, &hkDefault, &dw) != ERROR_SUCCESS)
				canEditDefaultPrefs = 0;
			if (hkLocal) RegCloseKey(hkLocal);
			if (hkDefault) RegCloseKey(hkDefault);

			if (!canEditDefaultPrefs) {
				MessageBox(NULL, sz_ID_CANNOT_EDIT_DEFAULT_PREFS, sz_ID_WINVNC_ERROR, MB_OK | MB_ICONEXCLAMATION);
				return;
			}
		}

		// Now, if the dialog is not already displayed, show it!
		if (!m_dlgvisible)
		{
			if (usersettings)
				vnclog.Print(LL_INTINFO, VNCLOG("show per-user Properties\n"));
			else
				vnclog.Print(LL_INTINFO, VNCLOG("show default system Properties\n"));

			// Load in the settings relevant to the user or system
			Load(usersettings);

			for (;;)
			{
				m_returncode_valid = FALSE;

				// Do the dialog box
				int result = DialogBoxParam(hAppInstance,
				    MAKEINTRESOURCE(IDD_PROPERTIES), 
				    NULL,
				    (DLGPROC) DialogProcPoll,
				    (LONG) this);

				if (!m_returncode_valid)
				    result = IDCANCEL;

				vnclog.Print(LL_INTINFO, VNCLOG("dialog result = %d\n"), result);

				if (result == -1)
				{
					// Dialog box failed, so quit
					PostQuitMessage(0);
					return;
				}
				break;
				omni_thread::sleep(4);
			}

			// Load in all the settings
			Load(TRUE);
		}
	}
}




BOOL CALLBACK
vncPropertiesPoll::DialogProcPoll(HWND hwnd,
						  UINT uMsg,
						  WPARAM wParam,
						  LPARAM lParam )
{
	// We use the dialog-box's USERDATA to store a _this pointer
	// This is set only once WM_INITDIALOG has been recieved, though!
	vncPropertiesPoll *_this = (vncPropertiesPoll *) GetWindowLong(hwnd, GWL_USERDATA);

	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			// Retrieve the Dialog box parameter and use it as a pointer
			// to the calling vncPropertiesPoll object
			SetWindowLong(hwnd, GWL_USERDATA, lParam);
			_this = (vncPropertiesPoll *) lParam;
			_this->m_dlgvisible = TRUE;

			// Set the dialog box's title to indicate which Properties we're editting
			if (_this->m_usersettings) {
				SetWindowText(hwnd, sz_ID_CURRENT_USER_PROP);
			} else {
				SetWindowText(hwnd, sz_ID_DEFAULT_SYST_PROP);
			}


			

			

		   
			// Modif sf@2002
		   HWND hTurboMode = GetDlgItem(hwnd, IDC_TURBOMODE);
           SendMessage(hTurboMode, BM_SETCHECK, _this->m_server->TurboMode(), 0);

			// Set the polling options
			HWND hDriver = GetDlgItem(hwnd, IDC_DRIVER);
			SendMessage(hDriver,
				BM_SETCHECK,
				_this->m_server->Driver(),0);

			if (OSVersion()==3) // If NT4 or below, no driver
			{
				SendMessage(hDriver,BM_SETCHECK,false,0);
				_this->m_server->Driver(false);
				EnableWindow(hDriver, false);
			}

			HWND hHook = GetDlgItem(hwnd, IDC_HOOK);
			SendMessage(hHook,
				BM_SETCHECK,
				_this->m_server->Hook(),
				0);

			HWND hPollFullScreen = GetDlgItem(hwnd, IDC_POLL_FULLSCREEN);
			SendMessage(hPollFullScreen,
				BM_SETCHECK,
				_this->m_server->PollFullScreen(),
				0);

			HWND hPollForeground = GetDlgItem(hwnd, IDC_POLL_FOREGROUND);
			SendMessage(hPollForeground,
				BM_SETCHECK,
				_this->m_server->PollForeground(),
				0);

			HWND hPollUnderCursor = GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR);
			SendMessage(hPollUnderCursor,
				BM_SETCHECK,
				_this->m_server->PollUnderCursor(),
				0);

			HWND hPollConsoleOnly = GetDlgItem(hwnd, IDC_CONSOLE_ONLY);
			SendMessage(hPollConsoleOnly,
				BM_SETCHECK,
				_this->m_server->PollConsoleOnly(),
				0);
			EnableWindow(hPollConsoleOnly,
				_this->m_server->PollUnderCursor() || _this->m_server->PollForeground()
				);

			HWND hPollOnEventOnly = GetDlgItem(hwnd, IDC_ONEVENT_ONLY);
			SendMessage(hPollOnEventOnly,
				BM_SETCHECK,
				_this->m_server->PollOnEventOnly(),
				0);
			EnableWindow(hPollOnEventOnly,
				_this->m_server->PollUnderCursor() || _this->m_server->PollForeground()
				);
			SetForegroundWindow(hwnd);

			return FALSE; // Because we've set the focus
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{

		case IDOK:
		case IDC_APPLY:
			{
				

				
				// Modif sf@2002
				HWND hTurboMode = GetDlgItem(hwnd, IDC_TURBOMODE);
				_this->m_server->TurboMode(SendMessage(hTurboMode, BM_GETCHECK, 0, 0) == BST_CHECKED);

				// Handle the polling stuff
				HWND hDriver = GetDlgItem(hwnd, IDC_DRIVER);
				bool result=(SendMessage(hDriver, BM_GETCHECK, 0, 0) == BST_CHECKED);
				if (result)
				{
#ifdef DRIVER
					_this->m_server->Driver(CheckVideoDriver(0));
#endif
				}

				else _this->m_server->Driver(false);
				HWND hHook = GetDlgItem(hwnd, IDC_HOOK);
				_this->m_server->Hook(
					SendMessage(hHook, BM_GETCHECK, 0, 0) == BST_CHECKED
					);
				HWND hPollFullScreen = GetDlgItem(hwnd, IDC_POLL_FULLSCREEN);
				_this->m_server->PollFullScreen(
					SendMessage(hPollFullScreen, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				HWND hPollForeground = GetDlgItem(hwnd, IDC_POLL_FOREGROUND);
				_this->m_server->PollForeground(
					SendMessage(hPollForeground, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				HWND hPollUnderCursor = GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR);
				_this->m_server->PollUnderCursor(
					SendMessage(hPollUnderCursor, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				HWND hPollConsoleOnly = GetDlgItem(hwnd, IDC_CONSOLE_ONLY);
				_this->m_server->PollConsoleOnly(
					SendMessage(hPollConsoleOnly, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				HWND hPollOnEventOnly = GetDlgItem(hwnd, IDC_ONEVENT_ONLY);
				_this->m_server->PollOnEventOnly(
					SendMessage(hPollOnEventOnly, BM_GETCHECK, 0, 0) == BST_CHECKED
					);


				// Was ok pressed?
				if (LOWORD(wParam) == IDOK)
				{
					// Yes, so close the dialog
					vnclog.Print(LL_INTINFO, VNCLOG("enddialog (OK)\n"));

					_this->m_returncode_valid = TRUE;

					EndDialog(hwnd, IDOK);
					_this->m_dlgvisible = FALSE;
				}

				_this->m_server->SetHookings();

				return TRUE;
			}

		case IDCANCEL:
			vnclog.Print(LL_INTINFO, VNCLOG("enddialog (CANCEL)\n"));

			_this->m_returncode_valid = TRUE;

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
#ifdef DRIVER
				CheckVideoDriver(1);
#endif
			}
			return TRUE;
		



		}
		break;
	}
	return 0;
}






void
vncPropertiesPoll::Load(BOOL usersettings)
{

	if (m_dlgvisible) {
		vnclog.Print(LL_INTWARN, VNCLOG("service helper invoked while Properties panel displayed\n"));
		return;
	}
	// Set the default user prefs
	vnclog.Print(LL_INTINFO, VNCLOG("clearing user settings\n"));
	m_pref_TurboMode = FALSE;
	m_pref_PollUnderCursor=FALSE;
	m_pref_PollForeground=TRUE;
	m_pref_PollFullScreen=TRUE;
	m_pref_PollConsoleOnly=FALSE;
	m_pref_PollOnEventOnly=TRUE;
#ifdef DRIVER
	m_pref_Driver=CheckVideoDriver(0);
#else
	m_pref_Driver=false;
#endif
	m_pref_Hook=TRUE;
	m_pref_Virtual=FALSE;

	// Make the loaded settings active..
	ApplyUserPrefs();

	// Note whether we loaded the user settings or just the default system settings
	m_usersettings = usersettings;
}


void
vncPropertiesPoll::ApplyUserPrefs()
{
	// APPLY THE CACHED PREFERENCES TO THE SERVER

	
    m_server->TurboMode(m_pref_TurboMode);
	// Polling prefs
	m_server->PollUnderCursor(m_pref_PollUnderCursor);
	m_server->PollForeground(m_pref_PollForeground);
	m_server->PollFullScreen(m_pref_PollFullScreen);
	m_server->PollConsoleOnly(m_pref_PollConsoleOnly);
	m_server->PollOnEventOnly(m_pref_PollOnEventOnly);
#ifdef DRIVER		
	if (CheckVideoDriver(0) && m_pref_Driver) m_server->Driver(m_pref_Driver);
	else m_server->Driver(false);
#else
	m_server->Driver(false);
#endif
	m_server->Hook(m_pref_Hook);
	m_server->Virtual(m_pref_Virtual);

}


#endif

