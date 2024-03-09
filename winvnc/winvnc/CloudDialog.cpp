/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
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


#include "CloudDialog.h"
#include "vncserver.h"
#include "SettingsManager.h"
#include "resource.h"

#ifdef _CLOUD
#include "./UdtCloudlib/proxy/Cloudthread.h"

extern HINSTANCE	hInstResDLL;

CloudDialog::CloudDialog()
{
	m_dlgvisible = FALSE;
}

CloudDialog::~CloudDialog()
{
}

void CloudDialog::LoadFromIniFile()
{
#ifndef SC_20
	settings->load();
#endif // SC_20
}
void CloudDialog::SaveToIniFile()
{
#ifndef SC_20
	settings->save();
#endif // SC_20
}

// Initialisation
void CloudDialog::Init(vncServer* server)
{
	// Save the server pointer
	m_server = server;
	LoadFromIniFile();
}

// Dialog box handling functions
void CloudDialog::Show(bool show, bool SC)
{
	this->SC = SC;
	if (show)
	{
		if (!m_dlgvisible)
		{
			//	[v1.0.2-jp1 fix]
			//DialogBoxParam(hAppInstance,
			DialogBoxParam(hInstResDLL, MAKEINTRESOURCE(IDD_DIALOGCLOUD), NULL, (DLGPROC)DialogProc, (LONG_PTR)this);
		}
	}
}

BOOL CALLBACK
CloudDialog::DialogProc(HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	// We use the dialog-box's USERDATA to store a _this pointer
	// This is set only once WM_INITDIALOG has been recieved, though!
#ifndef _X64
	CloudDialog* _this = (CloudDialog*)GetWindowLong(hwnd, GWL_USERDATA);
#else
	CloudDialog* _this = (CloudDialog*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
#endif // _X64
	switch (uMsg)
	{

	case WM_INITDIALOG:
	{
		// Retrieve the Dialog box parameter and use it as a pointer
		// to the calling vncProperties object
#ifndef _X64
		SetWindowLong(hwnd, GWL_USERDATA, lParam);
#else
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
#endif // _X64
		_this = (CloudDialog*)lParam;
		if (_this->SC) {
			ShowWindow(GetDlgItem(hwnd, IDC_CHECKCLOUD), false);
			ShowWindow(GetDlgItem(hwnd, IDOK), false);
		}
		SetDlgItemText(hwnd, IDC_CLOUDSERVER, settings->getCloudServer());
		SetDlgItemText(hwnd, IDC_CLOUDCODE, _this->m_server->code);
		SendMessage(GetDlgItem(hwnd, IDC_CHECKCLOUD), BM_SETCHECK, settings->getCloudEnabled(), 0);
		if (_this->m_server->isCloudThreadRunning()) {
			SetDlgItemText(hwnd, IDC_STARTCLOUD, "Stop");
			ShowWindow(GetDlgItem(hwnd, IDC_CLOUDCODE), true);
		}
		else {
			_this->m_server->cloudConnect(settings->getCloudEnabled(), settings->getCloudServer());
			SetDlgItemText(hwnd, IDC_STARTCLOUD, "Start");
			ShowWindow(GetDlgItem(hwnd, IDC_CLOUDCODE), false);
		}

		HFONT  hfont = CreateFont(
			24,                        // nHeight
			0,                         // nWidth
			0,                         // nEscapement
			0,                         // nOrientation
			FW_BOLD,                 // nWeight
			FALSE,                     // bItalic
			FALSE,                     // bUnderline
			0,                         // cStrikeOut
			ANSI_CHARSET,              // nCharSet
			OUT_DEFAULT_PRECIS,        // nOutPrecision
			CLIP_DEFAULT_PRECIS,       // nClipPrecision
			DEFAULT_QUALITY,           // nQuality
			DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
			_T("Arial"));                 // lpszFacename

		SendMessage(GetDlgItem(hwnd, IDC_CLOUDCODE), WM_SETFONT, WPARAM(hfont), TRUE);
		SetForegroundWindow(hwnd);
		SetTimer(hwnd, 120, 1000, NULL);
		_this->m_dlgvisible = TRUE;
		ShowWindow(GetDlgItem(hwnd, IDC_GREEN), false);
		ShowWindow(GetDlgItem(hwnd, IDC_YELLOW), false);
		ShowWindow(GetDlgItem(hwnd, IDC_RED), true);
		if (_this->SC) {
			SendMessage(GetDlgItem(hwnd, IDC_STARTCLOUD), BM_CLICK, 0, 0);
		}
		return TRUE;
	}
	case WM_TIMER:
		if (wParam == 120) {
			if (_this->m_server->isCloudThreadRunning()) {
				SetDlgItemText(hwnd, IDC_STARTCLOUD, "Stop");
				ShowWindow(GetDlgItem(hwnd, IDC_CLOUDCODE), true);
			}
			else {
				SetDlgItemText(hwnd, IDC_STARTCLOUD, "Start");
				ShowWindow(GetDlgItem(hwnd, IDC_CLOUDCODE), false);
			}
			SetDlgItemText(hwnd, IDC_EXTERNALIPADDRESS, _this->m_server->getExternalIpAddress());
			switch (_this->m_server->getStatus()) {
				case csConnected:
					ShowWindow(GetDlgItem(hwnd, IDC_GREEN), true);
					ShowWindow(GetDlgItem(hwnd, IDC_YELLOW), false);
					ShowWindow(GetDlgItem(hwnd, IDC_RED), false);
					break;
				case csOnline:	
				case csRendezvous:
					ShowWindow(GetDlgItem(hwnd, IDC_GREEN), false);
					ShowWindow(GetDlgItem(hwnd, IDC_YELLOW), true);
					ShowWindow(GetDlgItem(hwnd, IDC_RED), false);
					break;
				default:
					ShowWindow(GetDlgItem(hwnd, IDC_GREEN), false);
					ShowWindow(GetDlgItem(hwnd, IDC_YELLOW), false);
					ShowWindow(GetDlgItem(hwnd, IDC_RED), true);
					break;
			}
			

		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{

		case IDCANCEL:
			EndDialog(hwnd, TRUE);
			_this->m_dlgvisible = FALSE;
			return TRUE;
		case IDOK:
			TCHAR cloudServer[MAX_HOST_NAME_LEN];
			GetDlgItemText(hwnd, IDC_CLOUDSERVER, cloudServer, MAX_HOST_NAME_LEN);
			settings->setCloudServer(cloudServer);
			GetDlgItemText(hwnd, IDC_CLOUDCODE, _this->m_server->code, 18);
			settings->setCloudEnabled(SendMessage(GetDlgItem(hwnd, IDC_CHECKCLOUD), BM_GETCHECK, 0, 0) == BST_CHECKED);
			_this->SaveToIniFile();
			EndDialog(hwnd, TRUE);
			_this->m_dlgvisible = FALSE;
			return TRUE;

		case IDC_STARTCLOUD:
			ShowWindow(GetDlgItem(hwnd, IDC_CLOUDCODE), true);
			GetDlgItemText(hwnd, IDC_CLOUDSERVER, settings->getCloudServer(), MAX_HOST_NAME_LEN);
			if (!_this->m_server->isCloudThreadRunning()) {
				_this->m_server->cloudConnect(true, settings->getCloudServer());
			}
			else {				
				_this->m_server->cloudConnect(false, settings->getCloudServer());
			}
			return TRUE;
		}

		break;

	case WM_DESTROY:
		KillTimer(hwnd, 120);
		EndDialog(hwnd, FALSE);
		_this->m_dlgvisible = FALSE;
		return TRUE;
	}
	return 0;
}
#endif // _CLOUD
