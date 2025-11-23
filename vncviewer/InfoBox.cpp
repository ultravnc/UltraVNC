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
int g_error_nr2;
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
			SetDlgItemText(hwnd, IDC_BUILDTIME, isoTime);

            SetDlgItemText(hwnd, IDC_Message2, infomsg2);
			if ( (strcmp(infomsg2,"Your connection has been rejected.")==0)) g_error_nr2=1000;
			if ( (strcmp(infomsg2,"Local loop-back connections are disabled.")==0)) g_error_nr2=1001;
			ConvertStaticToHyperlink(hwnd, IDC_FORUMHYPERLINK);
			ConvertStaticToHyperlink(hwnd, IDC_WEBSITE);
			ConvertStaticToHyperlink(hwnd, IDC_GIT);
			ConvertStaticToHyperlink(hwnd, IDC_WEBDOWNLOAD);
			char version[50]{};
			char title[256]{};
			strcpy_s(title, "UltraVNC Viewer - ");
			strcat_s(title, GetVersionFromResource(version));
			SetDlgItemText(hwnd, IDC_UVVERSION2, title);
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
			ShellExecute(GetDesktopWindow(), "open", "https://forum.uvnc.com/", "", 0, SW_SHOWNORMAL);
		}
		if (LOWORD(wParam) == IDC_WEBSITE) {
			ShellExecute(GetDesktopWindow(), "open", "https://uvnc.com/", "", 0, SW_SHOWNORMAL);
		}
		if (LOWORD(wParam) == IDC_GIT) {
			ShellExecute(GetDesktopWindow(), "open", "https://github.com/ultravnc/UltraVNC", "", 0, SW_SHOWNORMAL);
		}
		if (LOWORD(wParam) == IDC_WEBDOWNLOAD) {
			ShellExecute(GetDesktopWindow(), "open", "https://uvnc.com/downloads/ultravnc.html", "", 0, SW_SHOWNORMAL);
		}
	}
	return FALSE;
}

void ShowMessageBox2(char *info,int error_nr)
{
	infomsg2 = info;
	g_error_nr2 = error_nr;
	int res = DialogBox(m_hInstResDLL, DIALOG_MAKEINTRESOURCE(IDD_APP_MESSAGE2), NULL, (DLGPROC) MessageDlgProc2);
}
	
