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

#include "stdhdrs.h"
#include "vncviewer.h"
#include "Hyperlinks.h"
#include "UltraVNCHelperFunctions.h"

char *infomsg2;
int g_error_nr2;
bool	g_disable_sponsor=false;
extern char buildtime[];

// Process the About dialog.
static LRESULT CALLBACK MessageDlgProc2(HWND hwnd, UINT iMsg, 
										   WPARAM wParam, LPARAM lParam) {
	switch (iMsg) {
	case WM_INITDIALOG:
		{
			//CentreWindow(hwnd);
			SetForegroundWindow(hwnd);            
			SetDlgItemText(hwnd, IDC_BUILDTIME, buildtime);
            SetDlgItemText(hwnd, IDC_Message2, infomsg2);
			if ( (strcmp(infomsg2,"Your connection has been rejected.")==0)) g_error_nr2=1000;
			if ( (strcmp(infomsg2,"Local loop-back connections are disabled.")==0)) g_error_nr2=1001;
			ConvertStaticToHyperlink(hwnd, IDC_FORUMHYPERLINK);
			ConvertStaticToHyperlink(hwnd, IDC_WEBSITE);
			ConvertStaticToHyperlink(hwnd, IDC_GIT);
			ConvertStaticToHyperlink(hwnd, IDC_WEBDOWNLOAD);
			char version[50]{};
			char title[256]{};
			strcpy_s(title, "ULtraVNC Viewer - ");
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
			ShellExecute(GetDesktopWindow(), "open", "https://forum.uvnc.com", "", 0, SW_SHOWNORMAL);
		}
		if (LOWORD(wParam) == IDC_WEBSITE) {
			ShellExecute(GetDesktopWindow(), "open", "https://uvnc.com", "", 0, SW_SHOWNORMAL);
		}
		if (LOWORD(wParam) == IDC_GIT) {
			ShellExecute(GetDesktopWindow(), "open", "https://github.com/ultravnc/ultravnc", "", 0, SW_SHOWNORMAL);
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
	int res = DialogBox(pApp->m_instance, DIALOG_MAKEINTRESOURCE(IDD_APP_MESSAGE2), NULL, (DLGPROC) MessageDlgProc2);
}
	
