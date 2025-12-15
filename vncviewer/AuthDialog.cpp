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
}

AuthDialog::~AuthDialog()
{
}

int AuthDialog::DoDialog(DialogType dialogType, TCHAR IN_host[MAX_HOST_NAME_LEN], int IN_port, char hex[24], char catchphrase[1024])
{
	TCHAR tempchar[10];
	strcpy_s(_host, IN_host);
	strcat_s(_host, ":");
	strcat_s(_host, _itoa(IN_port, tempchar, 10));
	this->dialogType = dialogType;
	strcpy(this->hex, hex);
	strcpy(this->catchphrase, catchphrase);
	switch (dialogType)
	{
	case dtUserPass:
		return DialogBoxParam(pApp->m_instance, DIALOG_MAKEINTRESOURCE(IDD_AUTH_DIALOG), NULL, (DLGPROC)DlgProc, (LONG_PTR)this);
	case dtPass:
		return DialogBoxParam(pApp->m_instance, DIALOG_MAKEINTRESOURCE(IDD_AUTH_DIALOG3), NULL, (DLGPROC)DlgProc1, (LONG_PTR)this);
	case  dtUserPassNotEncryption:
		return DialogBoxParam(pApp->m_instance, DIALOG_MAKEINTRESOURCE(IDD_AUTH_DIALOG2), NULL, (DLGPROC)DlgProc, (LONG_PTR)this);
	case dtPassUpgrade:
		return DialogBoxParam(pApp->m_instance, DIALOG_MAKEINTRESOURCE(IDD_AUTH_DIALOG1), NULL, (DLGPROC)DlgProc1, (LONG_PTR)this);
	case dtUserPassRSA:
		m_bPassphraseMode = true;
		return DialogBoxParam(pApp->m_instance, DIALOG_MAKEINTRESOURCE(IDD_AUTH_DIALOG4), NULL, (DLGPROC)DlgProc, (LONG_PTR)this);
	case dtPassRSA:
		m_bPassphraseMode = true;
		return DialogBoxParam(pApp->m_instance, DIALOG_MAKEINTRESOURCE(IDD_AUTH_DIALOG5), NULL, (DLGPROC)DlgProc1, (LONG_PTR)this);
	}
	return 0;
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
			strcat_s(tempchar, "   ");
			strcat_s(tempchar, _this->_host);
			SetWindowText(hwnd, tempchar);
			SetForegroundWindow(hwnd);
			SetDlgItemText(hwnd, IDC_CATCHPHRASE, _this->catchphrase);
			SetDlgItemText(hwnd, IDC_SIGNATURE, _this->hex);
			HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY));
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			return TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				UINT res= GetDlgItemText( hwnd,  IDC_PASSWD_EDIT,
					_this->m_passwd, 256);
				res= GetDlgItemText( hwnd,  IDD_DOMAIN,
					_this->m_domain, 256);
				res= GetDlgItemText( hwnd,  IDD_USER_NAME,
					_this->m_user, 256);
				
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
			strcat_s(tempchar, "   ");
			strcat_s(tempchar, _this->_host);
			SetWindowText(hwnd, tempchar);
			SetDlgItemText(hwnd, IDC_CATCHPHRASE, _this->catchphrase);
			SetDlgItemText(hwnd, IDC_SIGNATURE, _this->hex);
			HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY));
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			return TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				UINT res= GetDlgItemText( hwnd,  IDC_PASSWD_EDIT,
					_this->m_passwd, 256);
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
