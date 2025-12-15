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


#include "stdhdrs.h"
#include "vncviewer.h"
char *infomsg;
int g_error_nr;
// Process the About dialog.
static LRESULT CALLBACK MessageDlgProc(HWND hwnd, UINT iMsg, 
										   WPARAM wParam, LPARAM lParam) {
	switch (iMsg) {
	case WM_INITDIALOG:
		{
			CentreWindow(hwnd);
			SetForegroundWindow(hwnd);
            extern char buildtime[];
            SetDlgItemText(hwnd, IDC_BUILDTIME, infomsg);
			HWND button=GetDlgItem(hwnd,IDC_BUTTON1);
			HBITMAP hbmBkGnd;
		    hbmBkGnd = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP12),IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
			SendMessage( button, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,(LPARAM)hbmBkGnd );

			return TRUE;
		}
	case WM_CLOSE:
		EndDialog(hwnd, TRUE);
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			EndDialog(hwnd, TRUE);
		}
		if (LOWORD(wParam) == IDC_BUTTON1) {
			ShellExecute(GetDesktopWindow(), "open", "http://sponsor.uvnc.com/index.html", "", 0, SW_SHOWNORMAL);
		}
	}
	return FALSE;
}

void ShowMessageBox(char *info,int error_nr)
{
	infomsg=info;
	g_error_nr=error_nr;
	int res = DialogBox(pApp->m_instance, 
 		DIALOG_MAKEINTRESOURCE(IDD_APP_MESSAGE),
		NULL, (DLGPROC) MessageDlgProc);
}
	
