/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
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
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////
#include "stdhdrs.h"
#include "vncviewer.h"
#include "SessionDialog.h"
#include <ShlObj.h>
#include "common/win32_helpers.h"

BOOL CALLBACK DlgProcEncoders(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcKeyboardMouse(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcDisplay(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcMisc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcSecurity(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcQuickOptions(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcListen(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern bool g_disable_sponsor;
extern char sz_F1[64];
extern char sz_F3[64];
extern char sz_F4[64];
extern char sz_D1[64];
extern char sz_D2[64];

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
TCHAR* BrowseFolder(TCHAR* saved_path, HWND hwnd);
int gcd(int a, int b);
////////////////////////////////////////////////////////////////////////////////
/// TAB
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::InitTab(HWND hwnd)
{
	if (m_hTab)
		return;
	m_hTab = GetDlgItem(hwnd, IDC_TAB);
	TCITEM item;
	item.mask = TCIF_TEXT;
	item.pszText = "Encoders";
	TabCtrl_InsertItem(m_hTab, 0, &item);
	item.pszText = "Mouse and keyboard";
	TabCtrl_InsertItem(m_hTab, 1, &item);
	item.pszText = "Display";
	TabCtrl_InsertItem(m_hTab, 2, &item);
	item.pszText = "Misc";
	TabCtrl_InsertItem(m_hTab, 3, &item);
	item.pszText = "Security";
	TabCtrl_InsertItem(m_hTab, 4, &item);
	item.pszText = "Quick encoder";
	TabCtrl_InsertItem(m_hTab, 5, &item);
	item.pszText = "Listen mode";
	TabCtrl_InsertItem(m_hTab, 6, &item);
	hTabEncoders = CreateDialogParam(pApp->m_instance,
		MAKEINTRESOURCE(IDD_ENCODERS),
		hwnd,
		(DLGPROC)DlgProcEncoders,
		(LONG_PTR)this);
	hTabKeyboardMouse = CreateDialogParam(pApp->m_instance,
		MAKEINTRESOURCE(IDD_KEYBOARDMOUSE),
		hwnd,
		(DLGPROC)DlgProcKeyboardMouse,
		(LONG_PTR)this);
	hTabDisplay = CreateDialogParam(pApp->m_instance,
		MAKEINTRESOURCE(IDD_DISPLAY),
		hwnd,
		(DLGPROC)DlgProcDisplay,
		(LONG_PTR)this);
	hTabMisc = CreateDialogParam(pApp->m_instance,
		MAKEINTRESOURCE(IDD_MISC),
		hwnd,
		(DLGPROC)DlgProcMisc,
		(LONG_PTR)this);
	hTabSecurity = CreateDialogParam(pApp->m_instance,
		MAKEINTRESOURCE(IDD_SECURITY),
		hwnd,
		(DLGPROC)DlgProcSecurity,
		(LONG_PTR)this);
	hTabQuickOptions = CreateDialogParam(pApp->m_instance,
		MAKEINTRESOURCE(IDD_QUICKOPTIONS),
		hwnd,
		(DLGPROC)DlgProcQuickOptions,
		(LONG_PTR)this);
	hTabListen = CreateDialogParam(pApp->m_instance,
		MAKEINTRESOURCE(IDD_LISTEN),
		hwnd,
		(DLGPROC)DlgProcListen,
		(LONG_PTR)this);
	RECT rc;
	GetWindowRect(m_hTab, &rc);
	MapWindowPoints(NULL, hwnd, (POINT*)&rc, 2);
	TabCtrl_AdjustRect(m_hTab, FALSE, &rc);
	SetWindowPos(hTabEncoders, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_SHOWWINDOW);
	SetWindowPos(hTabKeyboardMouse, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_HIDEWINDOW);
	SetWindowPos(hTabDisplay, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_HIDEWINDOW);
	SetWindowPos(hTabMisc, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_HIDEWINDOW);
	SetWindowPos(hTabSecurity, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_HIDEWINDOW);
	SetWindowPos(hTabQuickOptions, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_HIDEWINDOW);
	SetWindowPos(hTabListen, HWND_TOP, rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_HIDEWINDOW);
}
////////////////////////////////////////////////////////////////////////////////
int SessionDialog::HandleNotify(HWND hwndDlg, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pn = (LPNMHDR)lParam;
	switch (pn->code) {
	case TCN_SELCHANGE:
		switch (pn->idFrom) {
		case IDC_TAB:
			int i = TabCtrl_GetCurFocus(m_hTab);
			switch (i) {
			case 0:
				ShowWindow(hTabEncoders, SW_SHOW);
				SetFocus(hTabEncoders);
				InitDlgProcEncoders();
				return 0;
			case 1:
				ShowWindow(hTabKeyboardMouse, SW_SHOW);
				SetFocus(hTabKeyboardMouse);
				return 0;
			case 2:
				ShowWindow(hTabDisplay, SW_SHOW);
				SetFocus(hTabDisplay);
				return 0;
			case 3:
				ShowWindow(hTabMisc, SW_SHOW);
				SetFocus(hTabMisc);
				return 0;
			case 4:
				ShowWindow(hTabSecurity, SW_SHOW);
				SetFocus(hTabSecurity);
				return 0;
			case 5:
				ShowWindow(hTabQuickOptions, SW_SHOW);
				SetFocus(hTabQuickOptions);
				return 0;
			case 6:
				ShowWindow(hTabListen, SW_SHOW);
				SetFocus(hTabListen);
				return 0;
			}
			return 0;
		}
		return 0;
	case TCN_SELCHANGING:
		switch (pn->idFrom) {
		case IDC_TAB:
			int i = TabCtrl_GetCurFocus(m_hTab);
			switch (i) {
			case 0:
				ShowWindow(hTabEncoders, SW_HIDE);
				break;
			case 1:
				ShowWindow(hTabKeyboardMouse, SW_HIDE);
				break;
			case 2:
				ShowWindow(hTabDisplay, SW_HIDE);
				//allowMonitorSpanning = SendMessage(GetDlgItem(hTabDisplay, IDC_ALLOWSPAN), BM_GETCHECK, 0, 0) == BST_CHECKED;
				break;
			case 3:
				ShowWindow(hTabMisc, SW_HIDE);
				break;
			case 4:
				ShowWindow(hTabSecurity, SW_HIDE);
				break;
			case 5:
				ShowWindow(hTabQuickOptions, SW_HIDE);
				HandleQuickOption(ReadQuickOptionsFromUI(this, hTabQuickOptions));
				break;
			case 6:
				ShowWindow(hTabListen, SW_HIDE);
				break;
			}
			return 0;
		}
		return 0;
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK DlgProcEncoders(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SessionDialog* _this = helper::SafeGetWindowUserData<SessionDialog>(hwnd);
	switch (uMsg)
	{
	case WM_INITDIALOG: {
		helper::SafeSetWindowUserData(hwnd, lParam);
		SessionDialog* _this = (SessionDialog*)lParam;
		_this->EncodersHwnd = hwnd;
		_this->InitDlgProcEncoders();
		return TRUE;
	}
	case WM_CTLCOLORDLG:
		return (INT_PTR)GetStockObject(WHITE_BRUSH);
	case WM_CTLCOLORSTATIC: {
		HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		SetBkMode(hdcStatic, TRANSPARENT);
		return (LONG)(INT_PTR)GetStockObject(WHITE_BRUSH);
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		case IDOK: {
			return TRUE; }

		default:
			_this->HandeleEncodersMessages(hwnd, wParam);
			break;
		}
	}
	return (INT_PTR)FALSE;
}
////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DlgProcKeyboardMouse(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SessionDialog* _this = helper::SafeGetWindowUserData<SessionDialog>(hwnd);
	switch (uMsg)
	{
	case WM_INITDIALOG: {
		helper::SafeSetWindowUserData(hwnd, lParam);
		SessionDialog* _this = (SessionDialog*)lParam;
		_this->KeyboardMouseHwnd = hwnd;
		_this->InitDlgProcKeyboardMouse();
		return TRUE;
	}
	case WM_CTLCOLORDLG:
		return (INT_PTR)GetStockObject(WHITE_BRUSH);
	case WM_CTLCOLORSTATIC: {
		HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		SetBkMode(hdcStatic, TRANSPARENT);
		return (LONG)(INT_PTR)GetStockObject(WHITE_BRUSH);
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		case IDOK: {
			return TRUE; }
		default:

			break;
		}
	}
	return (INT_PTR)FALSE;
}
////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DlgProcDisplay(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SessionDialog* _this = helper::SafeGetWindowUserData<SessionDialog>(hwnd);
	switch (uMsg)
	{
	case WM_INITDIALOG: {
		helper::SafeSetWindowUserData(hwnd, lParam);
		SessionDialog* _this = (SessionDialog*)lParam;
		_this->DisplayHwnd = hwnd;
		_this->InitDlgProcDisplay();
		return TRUE;
	}

	case WM_CTLCOLORDLG:
		return (INT_PTR)GetStockObject(WHITE_BRUSH);

	case WM_CTLCOLORSTATIC: {
		HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		SetBkMode(hdcStatic, TRANSPARENT);
		return (LONG)(INT_PTR)GetStockObject(WHITE_BRUSH);
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		case IDOK: {
			return TRUE; }
		case IDC_CHANGESERVER:
			_this->changeServerRes = (SendMessage(GetDlgItem(hwnd, IDC_CHANGESERVER), BM_GETCHECK, 0, 0) == BST_CHECKED);
			EnableWindow(GetDlgItem(hwnd, IDC_EXTENDDISPLAY), _this->changeServerRes);
			EnableWindow(GetDlgItem(hwnd, IDC_USERVIRT), _this->changeServerRes);
			EnableWindow(GetDlgItem(hwnd, IDC_ALLMONS), _this->changeServerRes);
			EnableWindow(GetDlgItem(hwnd, IDC_SLIDERRES), _this->changeServerRes);
			break;
		default:

			break;
		}
	case WM_HSCROLL:
	{
		if ((lParam != 0) &&
			(reinterpret_cast<HWND>(lParam) == GetDlgItem(hwnd, IDC_SLIDERRES)))
		{
			switch (LOWORD(wParam))
			{
			case SB_ENDSCROLL:
			case SB_LEFT:
			case SB_RIGHT:
			case SB_LINELEFT:
			case SB_LINERIGHT:
			case SB_PAGELEFT:
			case SB_PAGERIGHT:
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				map<pair<int, int>, int>::iterator itr;
				int pos = SendMessage(GetDlgItem(hwnd, IDC_SLIDERRES), TBM_GETPOS, 0, 0L);
				int it = 0;
				for (itr = _this->resolutionMap.begin(); itr != _this->resolutionMap.end(); itr++) {
					if (it == pos) {
						CHAR temp[250];
						sprintf_s(temp, "%d x %d", (itr->first).first, (itr->first).second);
						SetDlgItemText(hwnd, IDC_RES, temp);
						_this->requestedWidth = (itr->first).first;
						_this->requestedHeight = (itr->first).second;
						break;
					}

					it++;
				}
				break;
			}
		}
		break;
	}
	}
	return (INT_PTR)FALSE;
}
////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DlgProcMisc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SessionDialog* _this = helper::SafeGetWindowUserData<SessionDialog>(hwnd);
	switch (uMsg)
	{
	case WM_INITDIALOG: {
		helper::SafeSetWindowUserData(hwnd, lParam);
		SessionDialog* _this = (SessionDialog*)lParam;
		_this->MiscHwnd = hwnd;
		_this->InitDlgProcMisc();
		return TRUE;
	}
	case WM_CTLCOLORDLG:
		return (INT_PTR)GetStockObject(WHITE_BRUSH);
	case WM_CTLCOLORSTATIC: {
		HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		SetBkMode(hdcStatic, TRANSPARENT);
		return (LONG)(INT_PTR)GetStockObject(WHITE_BRUSH);
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		case IDOK: {
			UINT res = GetDlgItemText(hwnd, IDC_FOLDER, _this->folder, 256);
			res = GetDlgItemText(hwnd, IDC_PREFIX, _this->prefix, 56);
			if (_this->Shared)
				_this->reconnectcounter = GetDlgItemInt(hwnd, IDC_SERVER_RECON, NULL, TRUE);
			else
				_this->reconnectcounter = 0;
			if (_this->Shared)
				_this->autoReconnect = GetDlgItemInt(hwnd, IDC_SERVER_RECON_TIME, NULL, TRUE);
			else
				_this->autoReconnect = 0;

			_this->FTTimeout = GetDlgItemInt(hwnd, IDC_FTTIMEOUT, NULL, TRUE);

			HWND hsponsor = GetDlgItem(hwnd, IDC_CHECK1);
			if (SendMessage(hsponsor, BM_GETCHECK, 0, 0) == BST_CHECKED)
				g_disable_sponsor = true;
			else
				g_disable_sponsor = false;
			GetDlgItemText(hwnd, IDC_IMAGEFORMAT, _this->imageFormat, 56);
			return TRUE; }
		case IDC_BROWSE:
			_tcscpy_s(_this->folder, MAX_PATH, BrowseFolder(_this->folder, hwnd));
			SetDlgItemText(hwnd, IDC_FOLDER, _this->folder);
			return TRUE;
		default:

			break;
		}
	}
	return (INT_PTR)FALSE;
}
////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DlgProcSecurity(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SessionDialog* _this = helper::SafeGetWindowUserData<SessionDialog>(hwnd);
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		helper::SafeSetWindowUserData(hwnd, lParam);
		SessionDialog* _this = (SessionDialog*)lParam;
		_this->SecurityHwnd = hwnd;
		_this->InitDlgProcSecurity();
		return TRUE;
	}

	case WM_CTLCOLORDLG:
		return (INT_PTR)GetStockObject(WHITE_BRUSH);

	case WM_CTLCOLORSTATIC:
	{
		HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		SetBkMode(hdcStatic, TRANSPARENT);
		return (LONG)(INT_PTR)GetStockObject(WHITE_BRUSH);
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;

		case IDOK:
			return TRUE;

		case IDC_PLUGIN_CHECK:
		{
			HWND hUse = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
			BOOL enable = SendMessage(hUse, BM_GETCHECK, 0, 0) == BST_CHECKED;
			HWND hplugin = GetDlgItem(hwnd, IDC_PLUGINS_COMBO);
			EnableWindow(GetDlgItem(hwnd, IDC_PLUGINS_COMBO), enable);

			if (strcmp(_this->szDSMPluginFilename, "") != 0 && enable)
			{
				int pos = SendMessage(hplugin, CB_FINDSTRINGEXACT, -1,
					(LPARAM) & (_this->szDSMPluginFilename[0]));

				if (pos != CB_ERR)
				{
					SendMessage(hplugin, CB_SETCURSEL, pos, 0);
					HWND hUsePlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
					SendMessage(hUsePlugin, BM_SETCHECK, TRUE, 0);
					EnableWindow(GetDlgItem(hwnd, IDC_PLUGIN_BUTTON), TRUE); // sf@2009 - Enable plugin config button
					EnableWindow(GetDlgItem(hwnd, IDC_PLUGINS_COMBO), TRUE);
				}
			}
			else {
				EnableWindow(GetDlgItem(hwnd, IDC_PLUGIN_BUTTON), false); // sf@2009 - Enable plugin config button
			}
		}
		return TRUE;

		case IDC_PLUGIN_BUTTON:
		{
			HWND hPlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
			if (SendMessage(hPlugin, BM_GETCHECK, 0, 0) == BST_CHECKED) {
				TCHAR szPlugin[MAX_PATH];
				GetDlgItemText(hwnd, IDC_PLUGINS_COMBO, szPlugin, MAX_PATH);
				// sf@2003 - The config button can be clicked several times with
				// different selected plugins...
				bool fLoadIt = true;
				char szParams[64];
				strcpy_s(szParams, sz_F4);
				// If a plugin is already loaded, check if it is the same that the one
				// we want to load.
				if (_this->m_pDSMPlugin->IsLoaded()) {
					_this->m_pDSMPlugin->DescribePlugin();
					if (!_stricmp(_this->m_pDSMPlugin->GetPluginFileName(), szPlugin)) {
						fLoadIt = false;
						_this->m_pDSMPlugin->SetPluginParams(hwnd, szParams);
					}
					else {
						// Unload the previous plugin
						_this->m_pDSMPlugin->UnloadPlugin();
						fLoadIt = true;
					}
				}

				if (!fLoadIt) return TRUE;

				if (_this->m_pDSMPlugin->LoadPlugin(szPlugin, _this->listening))
				{
					// We don't know the password yet... no matter the plugin requires
					// it or not, we will provide it later (at plugin "real" startup)
					// Knowing the environnement ("viewer") right now can be usefull or
					// even mandatory for the plugin (specific params saving and so on...)
					// The plugin receives environnement info but isn't inited at this point
					_this->m_pDSMPlugin->SetPluginParams(hwnd, szParams);
				}
				else
				{
					MessageBox(hwnd, sz_F1, sz_F3, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
				}
			}
			return TRUE;
		}

		default:

			break;
		}
	}
	return (INT_PTR)FALSE;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DlgProcListen(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SessionDialog* _this = helper::SafeGetWindowUserData<SessionDialog>(hwnd);
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		helper::SafeSetWindowUserData(hwnd, lParam);
		SessionDialog* _this = (SessionDialog*)lParam;
		_this->ListenHwnd = hwnd;
		_this->InitDlgProcListen();
		return TRUE;
	}

	case WM_CTLCOLORDLG:
		return (INT_PTR)GetStockObject(WHITE_BRUSH);

	case WM_CTLCOLORSTATIC:
	{
		HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		SetBkMode(hdcStatic, TRANSPARENT);
		return (LONG)(INT_PTR)GetStockObject(WHITE_BRUSH);
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_STARTLISTEN:
			_this->StartListener();
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		case IDOK:
			return TRUE;
		default:

			break;
		}
	}
	return (INT_PTR)FALSE;
}
////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DlgProcQuickOptions(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SessionDialog* _this = helper::SafeGetWindowUserData<SessionDialog>(hwnd);
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		helper::SafeSetWindowUserData(hwnd, lParam);
		SessionDialog* _this = (SessionDialog*)lParam;
		_this->QuickOptionsHwnd = hwnd;
		_this->SetQuickOption(_this->quickoption);
		return TRUE;
	}

	case WM_CTLCOLORDLG:
		return (INT_PTR)GetStockObject(WHITE_BRUSH);

	case WM_CTLCOLORSTATIC:
	{
		HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		SetBkMode(hdcStatic, TRANSPARENT);
		return (LONG)(INT_PTR)GetStockObject(WHITE_BRUSH);
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		case IDOK:
			return TRUE;
		default:

			break;
		}
	}
	return (INT_PTR)FALSE;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::InitDlgProcEncoders()
{
	HWND hwnd = EncodersHwnd;
	HWND had = GetDlgItem(hwnd, IDC_AUTODETECT);
	SendMessage(had, BM_SETCHECK, autoDetect, 0);
	int i = 0;
	int nPreferredEncoding = PreferredEncodings.empty() ? rfbEncodingZRLE : PreferredEncodings[0];
	for (i = rfbEncodingRaw; i <= LASTENCODING; i++) {
		HWND hPref = GetDlgItem(hwnd, IDC_RAWRADIO + (i - rfbEncodingRaw));
		SendMessage(hPref, BM_SETCHECK, (i == nPreferredEncoding), 0);
		EnableWindow(hPref, UseEnc[i] && !autoDetect);
	}
	HWND hCopyRect = GetDlgItem(hwnd, ID_SESSION_SET_CRECT);
	SendMessage(hCopyRect, BM_SETCHECK, UseEnc[rfbEncodingCopyRect], 0);
	EnableWindow(hCopyRect, !autoDetect);
	HWND hAcl = GetDlgItem(hwnd, IDC_ALLOW_COMPRESSLEVEL);
	EnableWindow(hAcl, !autoDetect);
	SendMessage(hAcl, BM_SETCHECK, useCompressLevel, 0);
	HWND hCl = GetDlgItem(hwnd, IDC_COMPRESSLEVEL);
	EnableWindow(hCl, !autoDetect);
	HWND hAj = GetDlgItem(hwnd, IDC_ALLOW_JPEG);
	EnableWindow(hAj, !autoDetect);
	SendMessage(hAj, BM_SETCHECK, enableJpegCompression, 0);
	HWND hQl = GetDlgItem(hwnd, IDC_QUALITYLEVEL);
	EnableWindow(hQl, !autoDetect);
	SetDlgItemInt(hwnd, IDC_COMPRESSLEVEL, compressLevel, FALSE);
	SetDlgItemInt(hwnd, IDC_QUALITYLEVEL, jpegQualityLevel, FALSE);
	// sf@2005 - New Color depth choice
	HWND hColorMode = NULL;
	hColorMode = GetDlgItem(hwnd, IDC_FULLCOLORS_RADIO);
	SendMessage(hColorMode, BM_SETCHECK, false, 0);
	hColorMode = GetDlgItem(hwnd, IDC_256COLORS_RADIO);
	SendMessage(hColorMode, BM_SETCHECK, false, 0);
	hColorMode = GetDlgItem(hwnd, IDC_64COLORS_RADIO);
	SendMessage(hColorMode, BM_SETCHECK, false, 0);
	hColorMode = GetDlgItem(hwnd, IDC_8COLORS_RADIO);
	SendMessage(hColorMode, BM_SETCHECK, false, 0);
	hColorMode = GetDlgItem(hwnd, IDC_8GREYCOLORS_RADIO);
	SendMessage(hColorMode, BM_SETCHECK, false, 0);
	hColorMode = GetDlgItem(hwnd, IDC_4GREYCOLORS_RADIO);
	SendMessage(hColorMode, BM_SETCHECK, false, 0);
	hColorMode = GetDlgItem(hwnd, IDC_2GREYCOLORS_RADIO);
	SendMessage(hColorMode, BM_SETCHECK, false, 0);
	switch (Use8Bit) {
	case rfbPFFullColors:
		hColorMode = GetDlgItem(hwnd, IDC_FULLCOLORS_RADIO);
		break;
	case rfbPF256Colors:
		hColorMode = GetDlgItem(hwnd, IDC_256COLORS_RADIO);
		break;
	case rfbPF64Colors:
		hColorMode = GetDlgItem(hwnd, IDC_64COLORS_RADIO);
		break;
	case rfbPF8Colors:
		hColorMode = GetDlgItem(hwnd, IDC_8COLORS_RADIO);
		break;
	case rfbPF8GreyColors:
		hColorMode = GetDlgItem(hwnd, IDC_8GREYCOLORS_RADIO);
		break;
	case rfbPF4GreyColors:
		hColorMode = GetDlgItem(hwnd, IDC_4GREYCOLORS_RADIO);
		break;
	case rfbPF2GreyColors:
		hColorMode = GetDlgItem(hwnd, IDC_2GREYCOLORS_RADIO);
		break;
	}
	if (hColorMode)
		SendMessage(hColorMode, BM_SETCHECK, true, 0);
	// sf@2005 - New color depth choice
	hColorMode = GetDlgItem(hwnd, IDC_FULLCOLORS_RADIO);
	EnableWindow(hColorMode, !autoDetect);
	hColorMode = GetDlgItem(hwnd, IDC_256COLORS_RADIO);
	EnableWindow(hColorMode, !autoDetect);
	hColorMode = GetDlgItem(hwnd, IDC_64COLORS_RADIO);
	EnableWindow(hColorMode, !autoDetect);
	hColorMode = GetDlgItem(hwnd, IDC_8COLORS_RADIO);
	EnableWindow(hColorMode, !autoDetect);
	hColorMode = GetDlgItem(hwnd, IDC_8GREYCOLORS_RADIO);
	EnableWindow(hColorMode, !autoDetect);
	hColorMode = GetDlgItem(hwnd, IDC_4GREYCOLORS_RADIO);
	EnableWindow(hColorMode, !autoDetect);
	hColorMode = GetDlgItem(hwnd, IDC_2GREYCOLORS_RADIO);
	EnableWindow(hColorMode, !autoDetect);
	// Modif sf@2002 - Cache
	HWND hCache = GetDlgItem(hwnd, ID_SESSION_SET_CACHE);
	SendMessage(hCache, BM_SETCHECK, fEnableCache, 0);
	EnableWindow(hCache, !autoDetect);
	HWND hZstd = GetDlgItem(hwnd, IDC_ZSTD);
	SendMessage(hZstd, BM_SETCHECK, fEnableZstd, 0);

#ifndef _XZ
	HWND hxz = GetDlgItem(hwnd, IDC_XZRADIO);
	EnableWindow(hxz, false);
	ShowWindow(hxz, false);
	HWND hxzyw = GetDlgItem(hwnd, IDC_XZYWRADIO);
	EnableWindow(hxzyw, false);
	ShowWindow(hxzyw, false);
#endif
	//adzm 2010-07-04
	HWND hpreemptiveUpdates = GetDlgItem(hwnd, IDC_PREEMPTIVEUPDATES);
	SendMessage(hpreemptiveUpdates, BM_SETCHECK, preemptiveUpdates ? BST_CHECKED : BST_UNCHECKED, 0);
	EnableWindow(hpreemptiveUpdates, !autoDetect);
}
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::InitDlgProcKeyboardMouse()
{
	HWND hwnd = KeyboardMouseHwnd;
	HWND hSwap = GetDlgItem(hwnd, ID_SESSION_SWAPMOUSE);
	SendMessage(hSwap, BM_SETCHECK, SwapMouse, 0);
	HWND hViewOnly = GetDlgItem(hwnd, IDC_VIEWONLY);
	SendMessage(hViewOnly, BM_SETCHECK, ViewOnly, 0);
	SetDlgItemInt(hwnd, IDC_MOUSE_THROTTLE, throttleMouse, FALSE);
	HWND hRemoteCursorEnabled = GetDlgItem(hwnd, IDC_CSHAPE_ENABLE_RADIO);
	HWND hRemoteCursorIgnored = GetDlgItem(hwnd, IDC_CSHAPE_IGNORE_RADIO);
	HWND hRemoteCursorDisabled = GetDlgItem(hwnd, IDC_CSHAPE_DISABLE_RADIO);

	if (requestShapeUpdates && !ignoreShapeUpdates) {
		SendMessage(hRemoteCursorEnabled, BM_SETCHECK, true, 0);
		SendMessage(hRemoteCursorIgnored, BM_SETCHECK, false, 0);
		SendMessage(hRemoteCursorDisabled, BM_SETCHECK, false, 0);
	}
	else if (requestShapeUpdates) {
		SendMessage(hRemoteCursorEnabled, BM_SETCHECK, false, 0);
		SendMessage(hRemoteCursorIgnored, BM_SETCHECK, true, 0);
		SendMessage(hRemoteCursorDisabled, BM_SETCHECK, false, 0);
	}
	else {
		SendMessage(hRemoteCursorEnabled, BM_SETCHECK, false, 0);
		SendMessage(hRemoteCursorIgnored, BM_SETCHECK, false, 0);
		SendMessage(hRemoteCursorDisabled, BM_SETCHECK, true, 0);
	}

	HWND hEmulate = GetDlgItem(hwnd, IDC_EMULATECHECK);
	SendMessage(hEmulate, BM_SETCHECK, Emul3Buttons, 0);
	HWND hJapkeyboard = GetDlgItem(hwnd, IDC_JAPKEYBOARD);
	SendMessage(hJapkeyboard, BM_SETCHECK, JapKeyboard, 0);
	HWND hNoHotKeys = GetDlgItem(hwnd, IDC_NOHOTKEYS);
	SendMessage(hNoHotKeys, BM_SETCHECK, NoHotKeys, 0);
}
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::setDisplays()
{
	resolutionMap.clear();
	char primaryDevicename[100] = { 0 };
	DISPLAY_DEVICEA dd;
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	DWORD dev = 0;
	DWORD id = 0;
	int width, height;
	while (EnumDisplayDevicesA(0, dev, &dd, 0))
	{
		if (!(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) && (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
			&& (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)) {
			DISPLAY_DEVICEA ddMon;
			ZeroMemory(&ddMon, sizeof(ddMon));
			ddMon.cb = sizeof(ddMon);
			DWORD devMon = 0;
			while (EnumDisplayDevicesA(dd.DeviceName, devMon, &ddMon, 0)) {
				if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE)
					break;
				devMon++;
			}

			if (!*ddMon.DeviceString)
			{
				EnumDisplayDevicesA(dd.DeviceName, 0, &ddMon, 0);
				if (!*ddMon.DeviceString)
					strcpy_s(ddMon.DeviceString, "Default Monitor");
			}

			// get information about the display's position and the current display mode
			DEVMODEA dm;
			ZeroMemory(&dm, sizeof(dm));
			dm.dmSize = sizeof(dm);
			dm.dmDriverExtra = 0;

			if (EnumDisplaySettingsExA((char*)dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0) == FALSE)
				EnumDisplaySettingsExA((char*)dd.DeviceName, ENUM_REGISTRY_SETTINGS, &dm, 0);

			width = dm.dmPelsWidth;
			height = dm.dmPelsHeight;
			strcpy_s(primaryDevicename, (char*)dd.DeviceName);
			id++;
		}

		dev++;
	}
	DEVMODEA dm = { 0 };
	dm.dmSize = sizeof(dm);
	for (int iModeNum = 0; EnumDisplaySettingsA(primaryDevicename, iModeNum, &dm) != 0; iModeNum++)
	{
		if (dm.dmBitsPerPel == 32)
			resolutionMap.insert(make_pair(make_pair(dm.dmPelsWidth, dm.dmPelsHeight), dm.dmBitsPerPel));
	}
	HWND slider = GetDlgItem(DisplayHwnd, IDC_SLIDERRES);
	SendMessage(slider, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(0, resolutionMap.size() - 1));
	SendMessage(slider, TBM_SETTICFREQ, 1, 10);
	//int mapSize = resolutionMap.size() -1;
	map<pair<int, int>, int>::iterator itr;
	int it = 0;
	int pos = 0;
	if (requestedWidth != 0)
		width = requestedWidth;
	if (requestedHeight != 0)
		height = requestedHeight;

	for (itr = resolutionMap.begin(); itr != resolutionMap.end(); itr++) {
		if (width == (itr->first).first && height == (itr->first).second) {
			pos = it;
			break;
		}
		it++;
	}
	SendMessage(slider, TBM_SETPOS, true, pos);
	CHAR temp[250];
	sprintf_s(temp, "%d x %d", (itr->first).first, (itr->first).second);
	SetDlgItemText(DisplayHwnd, IDC_RES, temp);
}

void SessionDialog::InitDlgProcDisplay()
{
	HWND hwnd = DisplayHwnd;
	HWND hChangeServer = GetDlgItem(hwnd, IDC_CHANGESERVER);
	SendMessage(hChangeServer, BM_SETCHECK, changeServerRes, 0);
	
	HWND hallowMonitorSpanning = GetDlgItem(hwnd, IDC_ALLOWSPAN);
	SendMessage(hallowMonitorSpanning, BM_SETCHECK, allowMonitorSpanning, 0);

	HWND hExtendDisplay = GetDlgItem(hwnd, IDC_EXTENDDISPLAY);
	SendMessage(hExtendDisplay, BM_SETCHECK, extendDisplay, 0);

	HWND hUseVirt = GetDlgItem(hwnd, IDC_USERVIRT);
	SendMessage(hUseVirt, BM_SETCHECK, use_virt, 0);

	HWND hAllMons = GetDlgItem(hwnd, IDC_ALLMONS);
	SendMessage(hAllMons, BM_SETCHECK, use_allmonitors, 0);

	EnableWindow(hExtendDisplay, changeServerRes);
	EnableWindow(hUseVirt, changeServerRes);
	EnableWindow(hAllMons, changeServerRes);
	EnableWindow(GetDlgItem(hwnd, IDC_SLIDERRES) , changeServerRes);

	HWND hShowToolbar = GetDlgItem(hwnd, IDC_SHOWTOOLBAR);
	SendMessage(hShowToolbar, BM_SETCHECK, ShowToolbar, 0);

	HWND hAutoScaling = GetDlgItem(hwnd, IDC_SCALING);
	SendMessage(hAutoScaling, BM_SETCHECK, fAutoScaling, 0);

	int Scales[13] = { 25, 50, 75, 80, 85, 90, 95, 100, 125, 150, 200, 300, 400 };
	HWND hViewerScale = GetDlgItem(hwnd, IDC_SCALE_CB);
	char szPer[4];
	for (int i = 0; i <= 12; i++) {
		_itoa_s(Scales[i], szPer, 10);
		SendMessage(hViewerScale, CB_INSERTSTRING, (WPARAM)i, (LPARAM)(int FAR*)szPer);
	}
	SetDlgItemInt(hwnd, IDC_SCALE_CB, ((scale_num * 100) / scale_den), FALSE);
	SetDlgItemInt(hwnd, IDC_SERVER_SCALE, nServerScale, FALSE);

	HWND hFullScreen = GetDlgItem(hwnd, IDC_FULLSCREEN);
	SendMessage(hFullScreen, BM_SETCHECK, FullScreen, 0);

	HWND hDirectx = GetDlgItem(hwnd, IDC_DIRECTX);
	SendMessage(hDirectx, BM_SETCHECK, Directx, 0);
	HWND hSavePos = GetDlgItem(hwnd, IDC_SAVEPOS);
	SendMessage(hSavePos, BM_SETCHECK, SavePos, 0);
	HWND hSaveSize = GetDlgItem(hwnd, IDC_SAVESIZE);
	SendMessage(hSaveSize, BM_SETCHECK, SaveSize, 0);
	setDisplays();
}
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::InitDlgProcMisc()
{
	HWND hwnd = MiscHwnd;
	if (Shared)
		SetDlgItemInt(hwnd, IDC_SERVER_RECON, reconnectcounter, FALSE);
	else
		SetDlgItemInt(hwnd, IDC_SERVER_RECON, 0, FALSE);
	if (Shared)
		SetDlgItemInt(hwnd, IDC_SERVER_RECON_TIME, autoReconnect, FALSE);
	else
		SetDlgItemInt(hwnd, IDC_SERVER_RECON_TIME, 0, FALSE);
	SetDlgItemInt(hwnd, IDC_FTTIMEOUT, FTTimeout, FALSE);
	HWND hsponsor = GetDlgItem(hwnd, IDC_CHECK1);
	SendMessage(hsponsor, BM_SETCHECK, g_disable_sponsor, 1);
	SetDlgItemText(hwnd, IDC_FOLDER, folder);
	SetDlgItemText(hwnd, IDC_PREFIX, prefix);
	HWND hNoStatus = GetDlgItem(hwnd, IDC_HIDESTATUS);
	SendMessage(hNoStatus, BM_SETCHECK, NoStatus, 0);
	HWND hcomboscreen = GetDlgItem(hwnd, IDC_IMAGEFORMAT);
	SendMessage(hcomboscreen, CB_RESETCONTENT, 0, 0);
	SendMessage(hcomboscreen, CB_ADDSTRING, 0, (LPARAM)".jpeg");
	SendMessage(hcomboscreen, CB_ADDSTRING, 0, (LPARAM)".png");
	SendMessage(hcomboscreen, CB_ADDSTRING, 0, (LPARAM)".gif");
	SendMessage(hcomboscreen, CB_ADDSTRING, 0, (LPARAM)".bmp");
	if (strcmp(imageFormat, ".jpeg") == NULL)
		SendMessage(hcomboscreen, CB_SETCURSEL, 0, 0);
	if (strcmp(imageFormat, ".png") == NULL)
		SendMessage(hcomboscreen, CB_SETCURSEL, 1, 0);
	if (strcmp(imageFormat, ".gif") == NULL)
		SendMessage(hcomboscreen, CB_SETCURSEL, 2, 0);
	if (strcmp(imageFormat, ".bmp") == NULL)
		SendMessage(hcomboscreen, CB_SETCURSEL, 3, 0);
}
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::InitDlgProcSecurity()
{
	HWND hwnd = SecurityHwnd;
	InitPlugin(hwnd);
	HWND hDisableClip = GetDlgItem(hwnd, IDC_DISABLECLIPBOARD);
	SendMessage(hDisableClip, BM_SETCHECK, DisableClipboard, 0);
	HWND hShared = GetDlgItem(hwnd, IDC_SHARED);
	SendMessage(hShared, BM_SETCHECK, Shared, 0);
	EnableWindow(hShared, running);

	HWND hfRequireEncryption = GetDlgItem(hwnd, IDC_ONLYENCRYPTED);
	SendMessage(hfRequireEncryption, BM_SETCHECK, fRequireEncryption, 0);
	HWND hfAutoAcceptIncoming = GetDlgItem(hwnd, IDC_AUTOACCEPT);
	SendMessage(hfAutoAcceptIncoming, BM_SETCHECK, fAutoAcceptIncoming, 0);
	HWND hfAutoAcceptNoDSM = GetDlgItem(hwnd, IDC_AUTOACCEPTNOWARN);
	SendMessage(hfAutoAcceptNoDSM, BM_SETCHECK, fAutoAcceptNoDSM, 0);
	HWND hrestricted = GetDlgItem(hwnd, IDC_HIDEMENU);
	SendMessage(hrestricted, BM_SETCHECK, restricted, 0);
	HWND hfUseEncryption = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
	SendMessage(hfUseEncryption, BM_SETCHECK, fUseDSMPlugin, 0);
}
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::InitDlgProcListen()
{
	HWND hwnd = ListenHwnd;
	SetDlgItemInt(hwnd, IDC_LISTENPORT, listenport, FALSE);
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::ReadDlgProcListen()
{
	HWND hwnd = ListenHwnd;
	listenport = GetDlgItemInt(hwnd, IDC_LISTENPORT, NULL, TRUE);
}
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::ReadDlgProcEncoders()
{
	HWND hwnd = EncodersHwnd;
	HWND had = GetDlgItem(hwnd, IDC_AUTODETECT);
	autoDetect = (SendMessage(had, BM_GETCHECK, 0, 0) == BST_CHECKED);
	PreferredEncodings.clear();
	for (int i = rfbEncodingRaw; i <= LASTENCODING; i++) {
		HWND hPref = GetDlgItem(hwnd, IDC_RAWRADIO + i - rfbEncodingRaw);
		if (SendMessage(hPref, BM_GETCHECK, 0, 0) == BST_CHECKED)
			PreferredEncodings.push_back(i);
	}
	// sd@2005 - New Color depth choice
	HWND hColorMode = GetDlgItem(hwnd, IDC_FULLCOLORS_RADIO);
	if (SendMessage(hColorMode, BM_GETCHECK, 0, 0) == BST_CHECKED)
		Use8Bit = rfbPFFullColors;
	hColorMode = GetDlgItem(hwnd, IDC_256COLORS_RADIO);
	if (SendMessage(hColorMode, BM_GETCHECK, 0, 0) == BST_CHECKED)
		Use8Bit = rfbPF256Colors;
	hColorMode = GetDlgItem(hwnd, IDC_64COLORS_RADIO);
	if (SendMessage(hColorMode, BM_GETCHECK, 0, 0) == BST_CHECKED)
		Use8Bit = rfbPF64Colors;
	hColorMode = GetDlgItem(hwnd, IDC_8COLORS_RADIO);
	if (SendMessage(hColorMode, BM_GETCHECK, 0, 0) == BST_CHECKED)
		Use8Bit = rfbPF8Colors;
	hColorMode = GetDlgItem(hwnd, IDC_8GREYCOLORS_RADIO);
	if (SendMessage(hColorMode, BM_GETCHECK, 0, 0) == BST_CHECKED)
		Use8Bit = rfbPF8GreyColors;
	hColorMode = GetDlgItem(hwnd, IDC_4GREYCOLORS_RADIO);
	if (SendMessage(hColorMode, BM_GETCHECK, 0, 0) == BST_CHECKED)
		Use8Bit = rfbPF4GreyColors;
	hColorMode = GetDlgItem(hwnd, IDC_2GREYCOLORS_RADIO);
	if (SendMessage(hColorMode, BM_GETCHECK, 0, 0) == BST_CHECKED)
		Use8Bit = rfbPF2GreyColors;

	// Tight Specific
	HWND hAllowCompressLevel = GetDlgItem(hwnd, IDC_ALLOW_COMPRESSLEVEL);
	useCompressLevel = (SendMessage(hAllowCompressLevel, BM_GETCHECK, 0, 0) == BST_CHECKED);
	compressLevel = GetDlgItemInt(hwnd, IDC_COMPRESSLEVEL, NULL, TRUE);
	if (compressLevel < 0)
		compressLevel = 0;
	if (compressLevel > 9)
		compressLevel = 9;
	HWND hAllowJpeg = GetDlgItem(hwnd, IDC_ALLOW_JPEG);
	enableJpegCompression = (SendMessage(hAllowJpeg, BM_GETCHECK, 0, 0) == BST_CHECKED);
	jpegQualityLevel = GetDlgItemInt(hwnd, IDC_QUALITYLEVEL, NULL, TRUE);
	if (jpegQualityLevel < 0)
		jpegQualityLevel = 0;
	if (jpegQualityLevel > 9)
		jpegQualityLevel = 9;

	HWND hpreemptiveUpdates = GetDlgItem(hwnd, IDC_PREEMPTIVEUPDATES);
	preemptiveUpdates = (SendMessage(hpreemptiveUpdates, BM_GETCHECK, 0, 0) == BST_CHECKED) ? true : false;
	HWND hCopyRect = GetDlgItem(hwnd, ID_SESSION_SET_CRECT);
	UseEnc[rfbEncodingCopyRect] =
		(SendMessage(hCopyRect, BM_GETCHECK, 0, 0) == BST_CHECKED);
	HWND hCache = GetDlgItem(hwnd, ID_SESSION_SET_CACHE);
	fEnableCache =
		(SendMessage(hCache, BM_GETCHECK, 0, 0) == BST_CHECKED);
	HWND hZstd = GetDlgItem(hwnd, IDC_ZSTD);
	fEnableZstd =
		(SendMessage(hZstd, BM_GETCHECK, 0, 0) == BST_CHECKED);
}
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::ReadDlgProcKeyboardMouse()
{
	HWND hwnd = KeyboardMouseHwnd;
	HWND hViewOnly = GetDlgItem(hwnd, IDC_VIEWONLY);
	ViewOnly =
		(SendMessage(hViewOnly, BM_GETCHECK, 0, 0) == BST_CHECKED);
	HWND hEmulate = GetDlgItem(hwnd, IDC_EMULATECHECK);
	Emul3Buttons =
		(SendMessage(hEmulate, BM_GETCHECK, 0, 0) == BST_CHECKED);

	HWND hJapkeyboard = GetDlgItem(hwnd, IDC_JAPKEYBOARD);
	JapKeyboard =
		(SendMessage(hJapkeyboard, BM_GETCHECK, 0, 0) == BST_CHECKED);

	requestShapeUpdates = false;
	ignoreShapeUpdates = false;
	HWND hRemoteCursor = GetDlgItem(hwnd, IDC_CSHAPE_ENABLE_RADIO);
	if (SendMessage(hRemoteCursor, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		requestShapeUpdates = true;
	}
	else {
		hRemoteCursor = GetDlgItem(hwnd, IDC_CSHAPE_IGNORE_RADIO);
		if (SendMessage(hRemoteCursor, BM_GETCHECK, 0, 0) == BST_CHECKED) {
			requestShapeUpdates = true;
			ignoreShapeUpdates = true;
		}
	}
	HWND hSwap = GetDlgItem(hwnd, ID_SESSION_SWAPMOUSE);
	SwapMouse =
		(SendMessage(hSwap, BM_GETCHECK, 0, 0) == BST_CHECKED);
	BOOL bGotInt = FALSE;
	UINT nThrottle = GetDlgItemInt(hwnd, IDC_MOUSE_THROTTLE, &bGotInt, FALSE);
	if (bGotInt)
		throttleMouse = (int)nThrottle;
	HWND hNoHotKeys = GetDlgItem(hwnd, IDC_NOHOTKEYS);
	NoHotKeys =
		(SendMessage(hNoHotKeys, BM_GETCHECK, 0, 0) == BST_CHECKED);
}
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::ReadDlgProcDisplay()
{
	HWND hwnd = DisplayHwnd;
	HWND hShowToolbar = GetDlgItem(hwnd, IDC_SHOWTOOLBAR);
	ShowToolbar =
		(SendMessage(hShowToolbar, BM_GETCHECK, 0, 0) == BST_CHECKED);

	HWND hAutoScaling = GetDlgItem(hwnd, IDC_SCALING);
	fAutoScaling = (SendMessage(hAutoScaling, BM_GETCHECK, 0, 0) == BST_CHECKED);

	HWND hViewerScaling = GetDlgItem(hwnd, IDC_SCALE_CB);
	int nErr;
	int nPer = GetDlgItemInt(hwnd, IDC_SCALE_CB, &nErr, FALSE);
	if (nPer > 0) {
		scale_num = nPer;
		scale_den = 100;
	}
	scaling = !(scale_num == 100);

	if (scaling || fAutoScaling) {
		nServerScale = GetDlgItemInt(hwnd, IDC_SERVER_SCALE, NULL, TRUE);
		FixScaling();
	}
	else {
		scale_num = 1;
		scale_den = 1;
		// Modif sf@2002 - Server Scaling
		nServerScale = GetDlgItemInt(hwnd, IDC_SERVER_SCALE, NULL, TRUE);
		if (nServerScale < 1 || nServerScale > 9)
			nServerScale = 1;
	}
	HWND hFullScreen = GetDlgItem(hwnd, IDC_FULLSCREEN);
	FullScreen =
		(SendMessage(hFullScreen, BM_GETCHECK, 0, 0) == BST_CHECKED);

	HWND hSavePos = GetDlgItem(hwnd, IDC_SAVEPOS);
	SavePos =
		(SendMessage(hSavePos, BM_GETCHECK, 0, 0) == BST_CHECKED);

	HWND hSaveSize = GetDlgItem(hwnd, IDC_SAVESIZE);
	SaveSize =
		(SendMessage(hSaveSize, BM_GETCHECK, 0, 0) == BST_CHECKED);

	HWND hDirectx = GetDlgItem(hwnd, IDC_DIRECTX);
	Directx =
		(SendMessage(hDirectx, BM_GETCHECK, 0, 0) == BST_CHECKED);

	HWND hAllowMonitorSpanning = GetDlgItem(hwnd, IDC_ALLOWSPAN);
	allowMonitorSpanning =
		(SendMessage(hAllowMonitorSpanning, BM_GETCHECK, 0, 0) == BST_CHECKED);

	HWND hChangeServerRes = GetDlgItem(hwnd, IDC_CHANGESERVER);
	changeServerRes =
		(SendMessage(hChangeServerRes, BM_GETCHECK, 0, 0) == BST_CHECKED);

	HWND hExtendDisplay = GetDlgItem(hwnd, IDC_EXTENDDISPLAY);
	extendDisplay =
		(SendMessage(hExtendDisplay, BM_GETCHECK, 0, 0) == BST_CHECKED);

	HWND hUseVirt = GetDlgItem(hwnd, IDC_USERVIRT);
	use_virt =
		(SendMessage(hUseVirt, BM_GETCHECK, 0, 0) == BST_CHECKED);

	HWND hAllMons = GetDlgItem(hwnd, IDC_ALLMONS);
	use_allmonitors =
		(SendMessage(hAllMons, BM_GETCHECK, 0, 0) == BST_CHECKED);
}
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::ReadDlgProcMisc()
{
	HWND hwnd = MiscHwnd;
	UINT res = GetDlgItemText(hwnd, IDC_FOLDER, folder, 256);
	res = GetDlgItemText(hwnd, IDC_PREFIX, prefix, 56);
	if (Shared)
		reconnectcounter = GetDlgItemInt(hwnd, IDC_SERVER_RECON, NULL, TRUE);
	else
		reconnectcounter = 0;
	if (Shared)
		autoReconnect = GetDlgItemInt(hwnd, IDC_SERVER_RECON_TIME, NULL, TRUE);
	else
		autoReconnect = 0;

	FTTimeout = GetDlgItemInt(hwnd, IDC_FTTIMEOUT, NULL, TRUE);

	HWND hsponsor = GetDlgItem(hwnd, IDC_CHECK1);
	if (SendMessage(hsponsor, BM_GETCHECK, 0, 0) == BST_CHECKED)
		g_disable_sponsor = true;
	else
		g_disable_sponsor = false;

	HWND hNoStatus = GetDlgItem(hwnd, IDC_HIDESTATUS);
	NoStatus = (SendMessage(hNoStatus, BM_GETCHECK, 0, 0) == BST_CHECKED);
	GetDlgItemText(hwnd, IDC_IMAGEFORMAT, imageFormat, 56);
}
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::ReadDlgProcSecurity()
{
	HWND hwnd = SecurityHwnd;
	Shared = (SendMessage(GetDlgItem(hwnd, IDC_SHARED), BM_GETCHECK, 0, 0) == BST_CHECKED);
	DisableClipboard = (SendMessage(GetDlgItem(hwnd, IDC_DISABLECLIPBOARD), BM_GETCHECK, 0, 0) == BST_CHECKED);
	if (SendMessage(GetDlgItem(hwnd, IDC_PLUGIN_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED) {
		TCHAR szPlugin[MAX_PATH];
		GetDlgItemText(hwnd, IDC_PLUGINS_COMBO, szPlugin, MAX_PATH);
		fUseDSMPlugin = true;
		strcpy_s(szDSMPluginFilename, szPlugin);
	}
	else
		fUseDSMPlugin = false;
	fRequireEncryption = (SendMessage(GetDlgItem(hwnd, IDC_ONLYENCRYPTED), BM_GETCHECK, 0, 0) == BST_CHECKED);
	fAutoAcceptIncoming = (SendMessage(GetDlgItem(hwnd, IDC_AUTOACCEPT), BM_GETCHECK, 0, 0) == BST_CHECKED);
	fAutoAcceptNoDSM = (SendMessage(GetDlgItem(hwnd, IDC_AUTOACCEPTNOWARN), BM_GETCHECK, 0, 0) == BST_CHECKED);
	restricted = (SendMessage(GetDlgItem(hwnd, IDC_HIDEMENU), BM_GETCHECK, 0, 0) == BST_CHECKED);
}
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::ReadDlgProc()
{
	TCHAR tmphost[256];
	TCHAR hostname[256];
	HWND hwnd = SessHwnd;
	GetDlgItemText(hwnd, IDC_HOSTNAME_EDIT, hostname, 256);
	if (ParseDisplay(hostname, tmphost, 255, &m_port)) {
		for (size_t i = 0, len = strlen(tmphost); i < len; i++)
			tmphost[i] = toupper(tmphost[i]);
		_tcscpy_s(m_host_dialog, tmphost);
	}
	_tcscpy_s(m_proxyhost, "");
	GetDlgItemText(hwnd, IDC_PROXY_EDIT, hostname, 256);

	//adzm 2010-02-15
	if (strlen(hostname) > 0) {
		TCHAR actualProxy[256];
		strcpy_s(actualProxy, hostname);
		if (strncmp(tmphost, "ID", 2) == 0) {
			int numericId = m_port;
			int numberOfHosts = 1;
			for (int i = 0; i < (int)strlen(hostname); i++) {
				if (hostname[i] == ';') {
					numberOfHosts++;
				}
			}
			if (numberOfHosts > 1) {
				int modulo = numericId % numberOfHosts;
				char* szToken = strtok(hostname, ";");
				while (szToken) {
					if (modulo == 0) {
						strcpy_s(actualProxy, szToken);
						break;
					}
					modulo--;
					szToken = strtok(NULL, ";");
				}
			}
		}
		if (ParseDisplay(actualProxy, tmphost, 255, &m_proxyport)) {
			_tcscpy_s(m_proxyhost, tmphost);
		}
	}

	m_fUseProxy = SendMessage(GetDlgItem(hwnd, IDC_RADIOREPEATER), BM_GETCHECK, 0, 0) == BST_CHECKED;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int SessionDialog::HandeleEncodersMessages(HWND hwnd, WPARAM wParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_AUTODETECT: {
		bool ad = IsDlgButtonChecked(hwnd, IDC_AUTODETECT) ? true : false;
		for (int i = rfbEncodingRaw; i <= LASTENCODING; i++) {
			int aa = IDC_RAWRADIO + (i - rfbEncodingRaw);
			EnableWindow(GetDlgItem(hwnd, IDC_RAWRADIO + (i - rfbEncodingRaw)), UseEnc[i] && !ad);
		}
		EnableWindow(GetDlgItem(hwnd, ID_SESSION_SET_CRECT), !ad);
		EnableWindow(GetDlgItem(hwnd, IDC_FULLCOLORS_RADIO), !ad);
		EnableWindow(GetDlgItem(hwnd, IDC_256COLORS_RADIO), !ad);
		EnableWindow(GetDlgItem(hwnd, IDC_64COLORS_RADIO), !ad);
		EnableWindow(GetDlgItem(hwnd, IDC_8COLORS_RADIO), !ad);
		EnableWindow(GetDlgItem(hwnd, IDC_8GREYCOLORS_RADIO), !ad);
		EnableWindow(GetDlgItem(hwnd, IDC_4GREYCOLORS_RADIO), !ad);
		EnableWindow(GetDlgItem(hwnd, IDC_2GREYCOLORS_RADIO), !ad);
		EnableWindow(GetDlgItem(hwnd, ID_SESSION_SET_CACHE), !ad);
		EnableWindow(GetDlgItem(hwnd, IDC_ALLOW_COMPRESSLEVEL), !ad);
		EnableWindow(GetDlgItem(hwnd, IDC_COMPRESSLEVEL), !ad);
		EnableWindow(GetDlgItem(hwnd, IDC_ALLOW_JPEG), !ad);
		EnableWindow(GetDlgItem(hwnd, IDC_QUALITYLEVEL), !ad);
		EnableWindow(GetDlgItem(hwnd, IDC_PREEMPTIVEUPDATES), !ad);
		autoDetect = ad;
		SetQuickOption(ad == false ? 8 : 1);
	}
					   return TRUE;

	case IDC_ZLIBRADIO: {
		bool xor = IsDlgButtonChecked(hwnd, IDC_ZLIBRADIO) ? true : false;
		if (xor) {
			SendMessage(GetDlgItem(hwnd, ID_SESSION_SET_CACHE), BM_SETCHECK, true, 0);
		}
		return TRUE;
	}
	case IDC_ULTRA: {
		bool ultra = IsDlgButtonChecked(hwnd, IDC_ULTRA) ? true : false;
		return TRUE;
	}
	case IDC_256COLORS_RADIO:
	case IDC_64COLORS_RADIO:
	case IDC_8COLORS_RADIO:
	case IDC_8GREYCOLORS_RADIO:
	case IDC_4GREYCOLORS_RADIO:
	case IDC_2GREYCOLORS_RADIO: {
		bool ultra2 = IsDlgButtonChecked(hwnd, IDC_ULTRA2) ? true : false;
		if (ultra2) {
			SendMessage(GetDlgItem(hwnd, IDC_ULTRA2), BM_SETCHECK, false, 0);
			SendMessage(GetDlgItem(hwnd, IDC_ULTRA), BM_SETCHECK, true, 0);
		}
	}
							  break;
	case IDC_ULTRA2: {
		bool ultra2 = IsDlgButtonChecked(hwnd, IDC_ULTRA2) ? true : false;
		if (ultra2) {
			SendMessage(GetDlgItem(hwnd, IDC_FULLCOLORS_RADIO), BM_SETCHECK, true, 0);
			SendMessage(GetDlgItem(hwnd, IDC_256COLORS_RADIO), BM_SETCHECK, false, 0);
			SendMessage(GetDlgItem(hwnd, IDC_64COLORS_RADIO), BM_SETCHECK, false, 0);
			SendMessage(GetDlgItem(hwnd, IDC_8COLORS_RADIO), BM_SETCHECK, false, 0);
			SendMessage(GetDlgItem(hwnd, IDC_8GREYCOLORS_RADIO), BM_SETCHECK, false, 0);
			SendMessage(GetDlgItem(hwnd, IDC_4GREYCOLORS_RADIO), BM_SETCHECK, false, 0);
			SendMessage(GetDlgItem(hwnd, IDC_2GREYCOLORS_RADIO), BM_SETCHECK, false, 0);
		}
		return TRUE;
	}
	}
	return TRUE;
}
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::HandleQuickOption(int quickoption)
{
	switch (quickoption)
	{
	case 1:
		PreferredEncodings.clear();
		PreferredEncodings.push_back(rfbEncodingHextile);
		Use8Bit = rfbPFFullColors; //false;
		fEnableCache = false;
		autoDetect = true;
		break;
	case 2:
		PreferredEncodings.clear();
		PreferredEncodings.push_back(rfbEncodingHextile);
		Use8Bit = rfbPFFullColors; // false; // Max colors
		autoDetect = false;
		fEnableCache = false;
		break;
	case 3:
		PreferredEncodings.clear();
		PreferredEncodings.push_back(rfbEncodingZRLE);
		Use8Bit = rfbPF256Colors; //false;
		autoDetect = false;
		fEnableCache = false;
		break;
	case 4:
		PreferredEncodings.clear();
		PreferredEncodings.push_back(rfbEncodingZRLE);
		Use8Bit = rfbPF64Colors; //true;
		autoDetect = false;
		fEnableCache = true;
		break;
	case 5:
		PreferredEncodings.clear();
		PreferredEncodings.push_back(rfbEncodingZRLE);
		Use8Bit = rfbPF8Colors; //true;
		enableJpegCompression = false;
		autoDetect = false;
		fEnableCache = true;
		break;
	case 7:
		PreferredEncodings.clear();
		PreferredEncodings.push_back(rfbEncodingUltra);
		Use8Bit = rfbPFFullColors; //false; // Max colors
		autoDetect = false;
		UseEnc[rfbEncodingCopyRect] = false;
		fEnableCache = false;
		requestShapeUpdates = false;
		ignoreShapeUpdates = true;
		break;
	case 8:
		autoDetect = false;
		break;

	default:
		break;
	}
}
////////////////////////////////////////////////////////////////////////////////
int SessionDialog::SetQuickOption(int quickoption)
{
	HWND hwnd = QuickOptionsHwnd;
	SendMessage(GetDlgItem(hwnd, IDC_DYNAMIC), BM_SETCHECK, false, 0);
	SendMessage(GetDlgItem(hwnd, IDC_LAN_RB), BM_SETCHECK, false, 0);
	SendMessage(GetDlgItem(hwnd, IDC_ULTRA_LAN_RB), BM_SETCHECK, false, 0);
	SendMessage(GetDlgItem(hwnd, IDC_MEDIUM_RB), BM_SETCHECK, false, 0);
	SendMessage(GetDlgItem(hwnd, IDC_MODEM_RB), BM_SETCHECK, false, 0);
	SendMessage(GetDlgItem(hwnd, IDC_SLOW_RB), BM_SETCHECK, false, 0);
	SendMessage(GetDlgItem(hwnd, IDC_MANUAL), BM_SETCHECK, false, 0);

	// sf@2002 - Select Modem Option as default
	switch (quickoption) {
	case 1: // AUTO
		SendMessage(GetDlgItem(hwnd, IDC_DYNAMIC), BM_SETCHECK, true, 0);
		break;
	case 2: // LAN
		SendMessage(GetDlgItem(hwnd, IDC_LAN_RB), BM_SETCHECK, true, 0);
		break;
	case 3: // MEDIUM
		SendMessage(GetDlgItem(hwnd, IDC_MEDIUM_RB), BM_SETCHECK, true, 0);
		break;
	case 4: // MODEM
		SendMessage(GetDlgItem(hwnd, IDC_MODEM_RB), BM_SETCHECK, true, 0);
		break;
	case 5: // SLOW
		SendMessage(GetDlgItem(hwnd, IDC_SLOW_RB), BM_SETCHECK, true, 0);
		break;
	case 7: // LAN
		SendMessage(GetDlgItem(hwnd, IDC_ULTRA_LAN_RB), BM_SETCHECK, true, 0);
		break;
	default: // MANUAL
		SendMessage(GetDlgItem(hwnd, IDC_MANUAL), BM_SETCHECK, true, 0);
		break;
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
int SessionDialog::ReadQuickOptionsFromUI(SessionDialog* _this, HWND hwnd)
{
	// Auto Mode
	if ((SendMessage(GetDlgItem(hwnd, IDC_DYNAMIC), BM_GETCHECK, 0, 0) == BST_CHECKED))
		_this->quickoption = 1;
	// Options for LAN Mode
	if ((SendMessage(GetDlgItem(hwnd, IDC_ULTRA_LAN_RB), BM_GETCHECK, 0, 0) == BST_CHECKED))
		_this->quickoption = 7;
	// Options for LAN Mode
	if ((SendMessage(GetDlgItem(hwnd, IDC_LAN_RB), BM_GETCHECK, 0, 0) == BST_CHECKED))
		_this->quickoption = 2;
	// Options for Medium Mode
	if ((SendMessage(GetDlgItem(hwnd, IDC_MEDIUM_RB), BM_GETCHECK, 0, 0) == BST_CHECKED))
		_this->quickoption = 3;
	// Options for Modem Mode
	if ((SendMessage(GetDlgItem(hwnd, IDC_MODEM_RB), BM_GETCHECK, 0, 0) == BST_CHECKED))
		_this->quickoption = 4;
	// Options for Slow Mode
	if ((SendMessage(GetDlgItem(hwnd, IDC_SLOW_RB), BM_GETCHECK, 0, 0) == BST_CHECKED))
		_this->quickoption = 5;
	// Options for Manual
	if ((SendMessage(GetDlgItem(hwnd, IDC_MANUAL), BM_GETCHECK, 0, 0) == BST_CHECKED))
		_this->quickoption = 8;
	// Set the params depending on the selected QuickOption
	_this->m_pCC->HandleQuickOption();
	return _this->quickoption;
}
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::FixScaling()
{
	if (scale_num < 1 || scale_den < 1 || scale_num > 400 || scale_den > 100)
	{
		MessageBox(NULL, sz_D2, sz_D1, MB_OK | MB_TOPMOST | MB_ICONWARNING);
		scale_num = 1;
		scale_den = 1;
		scaling = false;
	}
	int g = gcd(scale_num, scale_den);
	scale_num /= g;
	scale_den /= g;

	// Modif sf@2002 - Server Scaling
	if (nServerScale < 1 || nServerScale > 9) nServerScale = 1;
}
////////////////////////////////////////////////////////////////////////////////
void SessionDialog::StartListener()
{
	SettingsFromUI();
	for (int i = rfbEncodingRaw; i <= LASTENCODING; i++) {
		m_pOpt->m_UseEnc[i] = UseEnc[i];
	}

	m_pOpt->m_PreferredEncodings.clear();
	for (int i = 0; i < PreferredEncodings.size(); i++)
		m_pOpt->m_PreferredEncodings.push_back(PreferredEncodings[i]);

	m_pOpt->autoDetect = autoDetect;
	m_pOpt->m_fExitCheck = fExitCheck;
	m_pOpt->m_fUseProxy = m_fUseProxy;
	m_pOpt->m_allowMonitorSpanning = allowMonitorSpanning;
	m_pOpt->m_ChangeServerRes = changeServerRes;
	m_pOpt->m_extendDisplay = extendDisplay;
	m_pOpt->m_use_virt = use_virt;
	m_pOpt->m_use_allmonitors = use_allmonitors;
	m_pOpt->m_requestedWidth = requestedWidth;
	m_pOpt->m_requestedHeight = requestedHeight;
	m_pOpt->m_SwapMouse = SwapMouse;
	m_pOpt->m_DisableClipboard = DisableClipboard;
	m_pOpt->m_Use8Bit = Use8Bit;
	m_pOpt->m_Shared = Shared;
	m_pOpt->m_running = running;
	m_pOpt->m_ViewOnly = ViewOnly;
	m_pOpt->m_ShowToolbar = ShowToolbar;
	m_pOpt->m_fAutoScaling = fAutoScaling;
	m_pOpt->m_scale_num = scale_num;
	m_pOpt->m_scale_den = scale_den;
	m_pOpt->m_nServerScale = nServerScale;
	m_pOpt->m_reconnectcounter = reconnectcounter;
	m_pOpt->m_autoReconnect = autoReconnect;
	m_pOpt->m_FTTimeout = FTTimeout;
	m_pOpt->m_listenPort = listenport;
	m_pOpt->m_fEnableCache = fEnableCache;
	m_pOpt->m_fEnableZstd = fEnableZstd;
	m_pOpt->m_useCompressLevel = useCompressLevel;
	m_pOpt->m_enableJpegCompression = enableJpegCompression;
	m_pOpt->m_compressLevel = compressLevel;
	m_pOpt->m_jpegQualityLevel = jpegQualityLevel;
	m_pOpt->m_throttleMouse = throttleMouse;
	m_pOpt->m_requestShapeUpdates = requestShapeUpdates;
	m_pOpt->m_ignoreShapeUpdates = ignoreShapeUpdates;
	m_pOpt->m_Emul3Buttons = Emul3Buttons;
	m_pOpt->m_JapKeyboard = JapKeyboard;
	m_pOpt->m_quickoption = quickoption;
	m_pOpt->m_preemptiveUpdates = preemptiveUpdates;
	m_pOpt->m_FullScreen = FullScreen;
	m_pOpt->m_Directx = Directx;
	m_pOpt->m_SavePos = SavePos;
	m_pOpt->m_SaveSize = SaveSize;
	m_pOpt->m_fUseDSMPlugin = fUseDSMPlugin;
	strcpy_s(m_pOpt->m_szDSMPluginFilename, szDSMPluginFilename);
	m_pOpt->m_listening = listening;
	m_pOpt->m_oldplugin = oldplugin;
	strcpy_s(m_pOpt->m_document_folder, folder);
	strcpy_s(m_pOpt->m_prefix, prefix);
	strcpy_s(m_pOpt->m_imageFormat, imageFormat);
	m_pOpt->m_scaling = scaling;
	m_pOpt->m_keepAliveInterval = keepAliveInterval;
	m_pOpt->m_fAutoAcceptIncoming = fAutoAcceptIncoming;
	m_pOpt->m_fAutoAcceptNoDSM = fAutoAcceptNoDSM;
	m_pOpt->m_fRequireEncryption = fRequireEncryption;
	m_pOpt->m_restricted = restricted;
	m_pOpt->m_NoStatus = NoStatus;
	m_pOpt->m_NoHotKeys = NoHotKeys;
	m_pCC->Save_Latest_Connection();
	char exePath[255];
	GetModuleFileName(NULL, exePath, 255);
	size_t exePathLen = strlen(exePath);
	for (size_t x = exePathLen; x > 0; x--)
	{
		if (exePath[x] == '\\')
			break;
		else
			exePath[x] = '\0';
	}
	char name[255];
	GetModuleFileName(NULL, name, 255);
	ShellExecute(NULL, "open", name, "-listen", exePath, SW_SHOW);
	exit(1);
}