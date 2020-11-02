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
char *infomsg2;
int g_error_nr2;
bool	g_disable_sponsor=false;
// Process the About dialog.
static LRESULT CALLBACK MessageDlgProc2(HWND hwnd, UINT iMsg, 
										   WPARAM wParam, LPARAM lParam) {
	switch (iMsg) {
	case WM_INITDIALOG:
		{
			//CentreWindow(hwnd);
			SetForegroundWindow(hwnd);
            extern char buildtime[];
            SetDlgItemText(hwnd, IDC_BUILDTIME, infomsg2);
			if ( (strcmp(infomsg2,"Your connection has been rejected.")==NULL)) g_error_nr2=1000;
			if ( (strcmp(infomsg2,"Local loop-back connections are disabled.")==NULL)) g_error_nr2=1001;

			if (g_error_nr2!=0)
			{
				HWND button=GetDlgItem(hwnd,IDC_BUTTON1);
				HBITMAP hbmBkGnd;
				hbmBkGnd = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP13),IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
				SendMessage( button, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,(LPARAM)hbmBkGnd );
			}
			else
			{
				HWND button=GetDlgItem(hwnd,IDC_BUTTON1);
				ShowWindow(button,SW_HIDE);
			}
				HWND button=GetDlgItem(hwnd,IDC_BUTTON2);
				HBITMAP hbmBkGnd;
				hbmBkGnd = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP14),IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
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
			ShellExecute(GetDesktopWindow(), "open", "https://www.uvnc.com", "", 0, SW_SHOWNORMAL);
			/*char link[256];
			char tempchar[10];
			strcpy_s(link,"https://www.uvnc.com");
			_itoa_s(g_error_nr2,tempchar,10);
			strcat_s(link,tempchar);
			strcat_s(link,".html");
			ShellExecute(GetDesktopWindow(), "open", link, "", 0, SW_SHOWNORMAL);*/
		}
		if (LOWORD(wParam) == IDC_BUTTON2) {
			ShellExecute(GetDesktopWindow(), "open", "https://www.uvnc2me.com", "", 0, SW_SHOWNORMAL);
		}
	}
	return FALSE;
}

void ShowMessageBox2(char *info,int error_nr)
{
	infomsg2=info;
	g_error_nr2=error_nr;
	if (g_disable_sponsor)
	{
	int res = DialogBox(pApp->m_instance, 
 		DIALOG_MAKEINTRESOURCE(IDD_APP_MESSAGE1),
		NULL, (DLGPROC) MessageDlgProc2);
	}
	else
	{
		int res = DialogBox(pApp->m_instance, 
 		DIALOG_MAKEINTRESOURCE(IDD_APP_MESSAGE2),
		NULL, (DLGPROC) MessageDlgProc2);
	}
}
	
