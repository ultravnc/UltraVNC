/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
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


// vncSetAuth.cpp

// Implementation of the About dialog!

#include "stdhdrs.h"

#include "winvnc.h"
#include "vncsetauth.h"
#include "common/win32_helpers.h"
#include "SettingsManager.h"

#define MAXSTRING 254

// [v1.0.2-jp1 fix] Load resouce from dll
extern HINSTANCE	hInstResDLL;

// Constructor/destructor
vncSetAuth::vncSetAuth()
{
	m_dlgvisible = FALSE;
}

// Initialisation
BOOL
vncSetAuth::Init(vncServer *server)
{
	m_server = server;
	m_dlgvisible = FALSE;
	return TRUE;
}

// Dialog box handling functions
void vncSetAuth::Show(BOOL show)
{
	if (show)
	{
		if (!m_dlgvisible)
		{
			// [v1.0.2-jp1 fix] Load resouce from dll
			//DialogBoxParam(hAppInstance,
			DialogBoxParam(hInstResDLL,
				MAKEINTRESOURCE(IDD_MSLOGON), 
				NULL,
				(DLGPROC) DialogProc,
				(LONG_PTR) this);
		}
	}
}

BOOL CALLBACK vncSetAuth::DialogProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam )
{
	// We use the dialog-box's USERDATA to store a _this pointer
	// This is set only once WM_INITDIALOG has been recieved, though!
    vncSetAuth *_this = helper::SafeGetWindowUserData<vncSetAuth>(hwnd);

	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			// Retrieve the Dialog box parameter and use it as a pointer
			// to the calling vncProperties object
            helper::SafeSetWindowUserData(hwnd, lParam);

			_this = (vncSetAuth *) lParam;
			SetDlgItemText(hwnd, IDC_GROUP1, settings->getGroup1());
			SetDlgItemText(hwnd, IDC_GROUP2, settings->getGroup2());
			SetDlgItemText(hwnd, IDC_GROUP3, settings->getGroup3());
			if (settings->getLocdom1() ==1 || settings->getLocdom1() ==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG1L);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG1L);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (settings->getLocdom1() ==2 || settings->getLocdom1() ==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG1D);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG1D);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (settings->getLocdom2() ==1 || settings->getLocdom2() ==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG2L);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG2L);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (settings->getLocdom2() ==2 || settings->getLocdom2() ==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG2D);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG2D);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (settings->getLocdom3() ==1 || settings->getLocdom3() ==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG3L);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG3L);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (settings->getLocdom3() ==2 || settings->getLocdom3() ==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG3D);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG3D);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}			
			// Show the dialog
			SetForegroundWindow(hwnd);

			_this->m_dlgvisible = TRUE;

			return TRUE;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{

		case IDCANCEL:
			EndDialog(hwnd, TRUE);
				_this->m_dlgvisible = FALSE;
				return TRUE;
		case IDOK:
			{
				int locdom1 = 0;
				int locdom2 = 0;
				int locdom3 = 0;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG1L),BM_GETCHECK,0,0) == BST_CHECKED) locdom1 = locdom1 +1;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG1D),BM_GETCHECK,0,0) == BST_CHECKED) locdom1 = locdom1 +2;

				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG2L),BM_GETCHECK,0,0) == BST_CHECKED) locdom2 = locdom2 + 1;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG2D),BM_GETCHECK,0,0) == BST_CHECKED) locdom2 = locdom2 + 2;

				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG3L),BM_GETCHECK,0,0) == BST_CHECKED) locdom3 = locdom3 + 1;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG3D),BM_GETCHECK,0,0) == BST_CHECKED) locdom3 = locdom3 + 2;
				settings->setLocdom1(locdom1);
				settings->setLocdom2(locdom2);
				settings->setLocdom3(locdom2);
				GetDlgItemText(hwnd, IDC_GROUP1, (LPSTR)settings->getGroup1(), 240);
				GetDlgItemText(hwnd, IDC_GROUP2, (LPSTR)settings->getGroup2(), 240);
				GetDlgItemText(hwnd, IDC_GROUP3, (LPSTR)settings->getGroup3(), 240);

				settings->save();
				
				EndDialog(hwnd, TRUE);
				_this->m_dlgvisible = FALSE;
				return TRUE;
			}
		}

		break;

	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		_this->m_dlgvisible = FALSE;
		return TRUE;
	}
	return 0;
}
