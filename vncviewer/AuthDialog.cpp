// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


// AuthDialog.cpp: implementation of the AuthDialog class.

#include "stdhdrs.h"
#include "vncviewer.h"
#include "AuthDialog.h"
#include "Exception.h"
#include <windowsx.h>
#include "common/win32_helpers.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

AuthDialog::AuthDialog()
{
	m_passwd[0]=__T('\0');
	//adzm 2010-05-12 - passphrase
	m_bPassphraseMode = false;
	m_savePassword = false;
	m_hwndStatus = NULL;
	m_className[0] = L'\0';
	m_statusWasVisible = false;
}

AuthDialog::~AuthDialog()
{
}

void AuthDialog::SetStatusWindow(HWND hwndStatus, const wchar_t* className)
{
	m_hwndStatus = hwndStatus;
	if (className)
		wcscpy_s(m_className, _countof(m_className), className);
	else
		m_className[0] = L'\0';
}

int AuthDialog::DoDialog(DialogType dialogType, TCHAR IN_host[MAX_HOST_NAME_LEN], int IN_port, char hex[24], char catchphrase[1024])
{
	TCHAR tempchar[10];
	_tcscpy_s(_host, _countof(_host), IN_host);
	_tcscat_s(_host, _countof(_host), _T(":"));
	_itot_s(IN_port, tempchar, _countof(tempchar), 10);
	_tcscat_s(_host, _countof(_host), tempchar);
	this->dialogType = dialogType;
	strcpy(this->hex, hex);
	strcpy(this->catchphrase, catchphrase);
	
	// Hide status window during authentication if classname is used
	m_statusWasVisible = false;
	if (wcslen(m_className) > 0 && m_hwndStatus && IsWindowVisible(m_hwndStatus)) {
		m_statusWasVisible = true;
		ShowWindow(m_hwndStatus, SW_HIDE);
	}
	
	extern HINSTANCE m_hInstResDLL;
	int result = 0;
	switch (dialogType)
	{
	case dtUserPass:
		result = DialogBoxParam(m_hInstResDLL, DIALOG_MAKEINTRESOURCE(IDD_AUTH_DIALOG), NULL, (DLGPROC)DlgProc, (LONG_PTR)this);
		break;
	case dtPass:
		result = DialogBoxParam(m_hInstResDLL, DIALOG_MAKEINTRESOURCE(IDD_AUTH_DIALOG3), NULL, (DLGPROC)DlgProc1, (LONG_PTR)this);
		break;
	case  dtUserPassNotEncryption:
		result = DialogBoxParam(m_hInstResDLL, DIALOG_MAKEINTRESOURCE(IDD_AUTH_DIALOG2), NULL, (DLGPROC)DlgProc, (LONG_PTR)this);
		break;
	case dtPassUpgrade:
		result = DialogBoxParam(m_hInstResDLL, DIALOG_MAKEINTRESOURCE(IDD_AUTH_DIALOG1), NULL, (DLGPROC)DlgProc1, (LONG_PTR)this);
		break;
	case dtUserPassRSA:
		m_bPassphraseMode = true;
		result = DialogBoxParam(m_hInstResDLL, DIALOG_MAKEINTRESOURCE(IDD_AUTH_DIALOG4), NULL, (DLGPROC)DlgProc, (LONG_PTR)this);
		break;
	case dtPassRSA:
		m_bPassphraseMode = true;
		result = DialogBoxParam(m_hInstResDLL, DIALOG_MAKEINTRESOURCE(IDD_AUTH_DIALOG5), NULL, (DLGPROC)DlgProc1, (LONG_PTR)this);
		break;
	}
	
	// Restore status window after authentication if classname is used
	if (m_statusWasVisible && m_hwndStatus) {
		ShowWindow(m_hwndStatus, SW_SHOW);
	}
	
	return result;
}

BOOL CALLBACK AuthDialog::DlgProc(  HWND hwnd,  UINT uMsg,  
									   WPARAM wParam, LPARAM lParam ) {
	// This is a static method, so we don't know which instantiation we're 
	// dealing with. But we can get a pseudo-this from the parameter to 
	// WM_INITDIALOG, which we therafter store with the window and retrieve
	// as follows:

    AuthDialog *_this = helper::SafeGetWindowUserData<AuthDialog>(hwnd); 

	switch (uMsg) {

	case WM_INITDIALOG:
		{
            helper::SafeSetWindowUserData(hwnd, lParam);

			_this = (AuthDialog *) lParam;
			
			if (_this->dialogType == dtUserPassRSA)
				Edit_LimitText(GetDlgItem(hwnd, IDC_PASSWD_EDIT), 256);
            else
				Edit_LimitText(GetDlgItem(hwnd, IDC_PASSWD_EDIT), 32);
			//CentreWindow(hwnd);
			TCHAR tempchar[MAX_HOST_NAME_LEN];
			GetWindowText(hwnd, tempchar, MAX_HOST_NAME_LEN);
			_tcscat_s(tempchar, _countof(tempchar), _T("   "));
			_tcscat_s(tempchar, _countof(tempchar), _this->_host);
			SetWindowText(hwnd, tempchar);
			SetForegroundWindow(hwnd);
			{ wchar_t _wcp[1024]; MultiByteToWideChar(CP_UTF8,0,_this->catchphrase,-1,_wcp,1024); SetDlgItemTextW(hwnd, IDC_CATCHPHRASE, _wcp); }
			{ wchar_t _whx[64]; MultiByteToWideChar(CP_UTF8,0,_this->hex,-1,_whx,64); SetDlgItemTextW(hwnd, IDC_SIGNATURE, _whx); }
			HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY));
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			return TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				UINT res= GetDlgItemTextA( hwnd,  IDC_PASSWD_EDIT,
					_this->m_passwd, 256);
				res= GetDlgItemTextA( hwnd,  IDD_DOMAIN,
					_this->m_domain, 256);
				res= GetDlgItemTextA( hwnd,  IDD_USER_NAME,
					_this->m_user, 256);
				_this->m_savePassword = (IsDlgButtonChecked(hwnd, IDC_SAVE_PASSWORD) == BST_CHECKED);
				
				EndDialog(hwnd, TRUE);

				return TRUE;
			}
		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return TRUE;
		}
		break;
	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		return TRUE;
	}
	return 0;
}

BOOL CALLBACK AuthDialog::DlgProc1(  HWND hwnd,  UINT uMsg,  
									   WPARAM wParam, LPARAM lParam ) {
	// This is a static method, so we don't know which instantiation we're 
	// dealing with. But we can get a pseudo-this from the parameter to 
	// WM_INITDIALOG, which we therafter store with the window and retrieve
	// as follows:
    AuthDialog *_this = helper::SafeGetWindowUserData<AuthDialog>(hwnd); 

	switch (uMsg) {

	case WM_INITDIALOG:
		{           
			_this = (AuthDialog *) lParam;
			 helper::SafeSetWindowUserData(hwnd, lParam);
			//adzm 2010-05-12 - passphrase
			Edit_LimitText(GetDlgItem(hwnd, IDC_PASSWD_EDIT), _this->m_bPassphraseMode ? 128 : 8);
			//CentreWindow(hwnd);
			TCHAR tempchar[MAX_HOST_NAME_LEN];
			GetWindowText(hwnd, tempchar, MAX_HOST_NAME_LEN);
			_tcscat_s(tempchar, _countof(tempchar), _T("   "));
			_tcscat_s(tempchar, _countof(tempchar), _this->_host);
			SetWindowText(hwnd, tempchar);
			{ wchar_t _wcp[1024]; MultiByteToWideChar(CP_UTF8,0,_this->catchphrase,-1,_wcp,1024); SetDlgItemTextW(hwnd, IDC_CATCHPHRASE, _wcp); }
			{ wchar_t _whx[64]; MultiByteToWideChar(CP_UTF8,0,_this->hex,-1,_whx,64); SetDlgItemTextW(hwnd, IDC_SIGNATURE, _whx); }
			HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY));
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			return TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				UINT res= GetDlgItemTextA( hwnd,  IDC_PASSWD_EDIT,
					_this->m_passwd, 256);
				_this->m_savePassword = (IsDlgButtonChecked(hwnd, IDC_SAVE_PASSWORD) == BST_CHECKED);
				EndDialog(hwnd, TRUE);

				return TRUE;
			}
		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return TRUE;
		}
		break;
	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		return TRUE;
	}
	return 0;
}
