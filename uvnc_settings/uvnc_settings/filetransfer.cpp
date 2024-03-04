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


#include "stdafx.h"
#include "resource.h"

extern LONG EnableRemoteInputs;
extern LONG DisableLocalInputs;//
extern LONG EnableJapInput;//
extern LONG FileTransferEnabled;//
extern LONG FTUserImpersonation;//
extern LONG BlankMonitorEnabled;//
extern LONG BlankInputsOnly; //PGM
extern LONG FTTimeout;//

bool initdone3=false;
BOOL CALLBACK DlgProcOFT(HWND hwnd, UINT uMsg,
											   WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) {
		
	case WM_INITDIALOG: 
		{	
			initdone3=false;
			SendMessage(GetDlgItem(hwnd, IDC_FILETRANSFER), BM_SETCHECK, FileTransferEnabled, 0);
			SendMessage(GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK), BM_SETCHECK, FTUserImpersonation, 0);
			SendMessage(GetDlgItem(hwnd, IDC_BLANK), BM_SETCHECK, BlankMonitorEnabled, 0);
			SendMessage(GetDlgItem(hwnd, IDC_BLANK2), BM_SETCHECK, BlankInputsOnly, 0); //PGM
			SendMessage(GetDlgItem(hwnd, IDC_JAP_INPUTS), BM_SETCHECK, EnableJapInput, 0);

			SendMessage(GetDlgItem(hwnd, IDC_DISABLE_LOCAL_INPUTS), BM_SETCHECK, DisableLocalInputs, 0);
			SendMessage(GetDlgItem(hwnd, IDC_JAP_INPUTS), BM_SETCHECK, EnableJapInput, 0);
			SendMessage(GetDlgItem(hwnd, IDC_DISABLE_INPUTS), BM_SETCHECK, !EnableRemoteInputs, 0);

			SetDlgItemInt(hwnd, IDC_FTTIMEOUT, FTTimeout, FALSE);

			EnableWindow(GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK), FileTransferEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_FTTIMEOUT), FileTransferEnabled);
			
			initdone3=true;
			return TRUE;
		}
	
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{
		case IDC_HELP2:
			if (lParam==2)
			
{
			char link[256];
			strcpy(link,"https://uvnc.com/webhelp/");
			strcat(link,"input");
			strcat(link,".html");
			ShellExecute(GetDesktopWindow(), "open", link, "", 0, SW_SHOWNORMAL);
			}
			break;
		case IDOK:

			EnableRemoteInputs= !SendDlgItemMessage(hwnd, IDC_DISABLE_INPUTS, BM_GETCHECK, 0, 0);
			DisableLocalInputs= (LONG)SendDlgItemMessage(hwnd, IDC_DISABLE_LOCAL_INPUTS, BM_GETCHECK, 0, 0);
			EnableJapInput= (LONG)SendDlgItemMessage(hwnd, IDC_JAP_INPUTS, BM_GETCHECK, 0, 0);
			FileTransferEnabled= (LONG)SendDlgItemMessage(hwnd, IDC_FILETRANSFER, BM_GETCHECK, 0, 0);
			FTUserImpersonation= (LONG)SendDlgItemMessage(hwnd, IDC_FTUSERIMPERSONATION_CHECK, BM_GETCHECK, 0, 0);
			BlankMonitorEnabled= (LONG)SendDlgItemMessage(hwnd, IDC_BLANK, BM_GETCHECK, 0, 0);
			BlankInputsOnly= (LONG)SendDlgItemMessage(hwnd, IDC_BLANK2, BM_GETCHECK, 0, 0); //PGM
			BOOL ok1;
			FTTimeout=GetDlgItemInt(hwnd, IDC_FTTIMEOUT, &ok1, TRUE);
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		case IDC_FILETRANSFER:
			FileTransferEnabled= (LONG)SendDlgItemMessage(hwnd, IDC_FILETRANSFER, BM_GETCHECK, 0, 0);
			EnableWindow(GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK), FileTransferEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_FTTIMEOUT), FileTransferEnabled);
			break;
		case IDC_BLANK:
			BlankMonitorEnabled= (LONG)SendDlgItemMessage(hwnd, IDC_BLANK, BM_GETCHECK, 0, 0);
			EnableWindow(GetDlgItem(hwnd, IDC_BLANK2), BlankMonitorEnabled); //PGM
			if (!BlankMonitorEnabled) //PGM
				SendMessage(GetDlgItem(hwnd, IDC_BLANK2), BM_SETCHECK, BlankMonitorEnabled, 0); //PGM
			BlankInputsOnly= (LONG)SendDlgItemMessage(hwnd, IDC_BLANK2, BM_GETCHECK, 0, 0); //PGM
			break;
		case IDC_BLANK2: //PGM
			BlankInputsOnly= (LONG)SendDlgItemMessage(hwnd, IDC_BLANK2, BM_GETCHECK, 0, 0); //PGM
			break; //PGM
		}
		return 0;	
	}

	return 0;
}