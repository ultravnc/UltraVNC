// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include "stdhdrs.h"
#include "vncviewer.h"
#include "../common/Hyperlinks.h"
#include "UltraVNCHelperFunctions.h"

char *infomsg2;
const wchar_t *infomsg2_w;
int g_error_nr2;
bool use_wide_msg2 = false;
bool	g_disable_sponsor=false;
extern char buildtime[];
void convertToISO8601(const char* input, char* output, size_t size);

// Process the About dialog.
static LRESULT CALLBACK MessageDlgProc2(HWND hwnd, UINT iMsg, 
										   WPARAM wParam, LPARAM lParam) {
	switch (iMsg) {
	case WM_INITDIALOG:
		{
			HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY));
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			//CentreWindow(hwnd);
			SetForegroundWindow(hwnd);            
			SetForegroundWindow(hwnd);
			extern char buildtime[];
			char isoTime[20];  // Buffer for ISO output
			convertToISO8601(buildtime, isoTime, sizeof(isoTime));
			wchar_t wisoTime[20];
			MultiByteToWideChar(CP_ACP, 0, isoTime, -1, wisoTime, 20);
			SetDlgItemTextW(hwnd, IDC_BUILDTIME, wisoTime);

            if (use_wide_msg2) {
				SetDlgItemTextW(hwnd, IDC_Message2, infomsg2_w);
				if (_wcsicmp(infomsg2_w, L"Your connection has been rejected.") == 0) g_error_nr2 = 1000;
				if (_wcsicmp(infomsg2_w, L"Local loop-back connections are disabled.") == 0) g_error_nr2 = 1001;
			} else {
				wchar_t winfomsg2[1024]; MultiByteToWideChar(CP_ACP,0,infomsg2,-1,winfomsg2,1024); SetDlgItemTextW(hwnd, IDC_Message2, winfomsg2);
				if ( (strcmp(infomsg2,"Your connection has been rejected.")==0)) g_error_nr2=1000;
				if ( (strcmp(infomsg2,"Local loop-back connections are disabled.")==0)) g_error_nr2=1001;
			}
			ConvertStaticToHyperlink(hwnd, IDC_FORUMHYPERLINK);
			ConvertStaticToHyperlink(hwnd, IDC_WEBSITE);
			ConvertStaticToHyperlink(hwnd, IDC_GIT);
			ConvertStaticToHyperlink(hwnd, IDC_WEBDOWNLOAD);
			char version[50]{};
			char title[256]{};
			strcpy_s(title, "UltraVNC Viewer - ");
			strcat_s(title, GetVersionFromResource(version));
			wchar_t wtitle[256];
			MultiByteToWideChar(CP_ACP, 0, title, -1, wtitle, 256);
			SetDlgItemTextW(hwnd, IDC_UVVERSION2, wtitle);
			return TRUE;
		}
	case WM_CLOSE:
		EndDialog(hwnd, TRUE);
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			EndDialog(hwnd, TRUE);
		}
		if (LOWORD(wParam) == IDC_FORUMHYPERLINK) {
			ShellExecuteW(GetDesktopWindow(), L"open", L"https://forum.uvnc.com/", L"", 0, SW_SHOWNORMAL);
		}
		if (LOWORD(wParam) == IDC_WEBSITE) {
			ShellExecuteW(GetDesktopWindow(), L"open", L"https://uvnc.com/", L"", 0, SW_SHOWNORMAL);
		}
		if (LOWORD(wParam) == IDC_GIT) {
			ShellExecuteW(GetDesktopWindow(), L"open", L"https://github.com/ultravnc/UltraVNC", L"", 0, SW_SHOWNORMAL);
		}
		if (LOWORD(wParam) == IDC_WEBDOWNLOAD) {
			ShellExecuteW(GetDesktopWindow(), L"open", L"https://uvnc.com/downloads/ultravnc.html", L"", 0, SW_SHOWNORMAL);
		}
	}
	return FALSE;
}

void ShowMessageBox2(char *info,int error_nr)
{
	use_wide_msg2 = false;
	infomsg2 = info;
	g_error_nr2 = error_nr;
	int res = DialogBox(m_hInstResDLL, DIALOG_MAKEINTRESOURCE(IDD_APP_MESSAGE2), NULL, (DLGPROC) MessageDlgProc2);
}

void ShowMessageBox2(const wchar_t *info, int error_nr)
{
	use_wide_msg2 = true;
	infomsg2_w = info;
	g_error_nr2 = error_nr;
	int res = DialogBox(m_hInstResDLL, DIALOG_MAKEINTRESOURCE(IDD_APP_MESSAGE2), NULL, (DLGPROC) MessageDlgProc2);
}
	
