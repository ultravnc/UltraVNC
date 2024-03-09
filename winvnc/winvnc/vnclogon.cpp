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
#include "resource.h"
#include <omnithread.h>
#include "vncdesktop.h"
#include "vnclogon.h"


extern HINSTANCE	hAppInstance;
extern BOOL WINAPI SSPLogonUser(LPTSTR szDomain, LPTSTR szUser, LPTSTR szPassword);
TCHAR szUsername[64];
TCHAR szPassword[64];
TCHAR szDomain[64];

// [v1.0.2-jp1 fix] Load resouce from dll
extern HINSTANCE	hInstResDLL;

BOOL vncLogonThread::Init(vncDesktop *desktop)
{
	m_desktop = desktop;
	m_desktop->logon=1;
	logonhwnd=NULL;
	rect.left=0;
	rect.top=0;
	rect.right=32;
	rect.bottom=32;
	start_undetached();
	return TRUE;
}

void *vncLogonThread::run_undetached(void * arg)
{
	int logonstatus;
	logonstatus=CreateLogonWindow(hAppInstance);
	m_desktop->logon=logonstatus;

	return NULL;
}
BOOL CALLBACK  
vncLogonThread::LogonDlgProc(HWND hDlg,
							 UINT Message,
							 WPARAM wParam,
							 LPARAM lParam)
{
 TCHAR TempString[63];
 vncLogonThread *_this = (vncLogonThread *) GetWindowLong(hDlg, GWL_USERDATA);
  switch (Message)
    {
    case WM_INITDIALOG:
		_this = (vncLogonThread *) lParam;
      _this->CenterWindow(hDlg);
	  SetDlgItemText(hDlg, IDD_DOMAIN,TEXT("."));
      SetDlgItemText(hDlg, IDD_USER_NAME,TEXT(""));
      SetDlgItemText(hDlg, IDD_PASSWORD, TEXT(""));
      SetFocus(GetDlgItem(hDlg, IDD_USER_NAME));

      return(TRUE);

    case WM_COMMAND:
      if (LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, 0);
			}
      if (LOWORD(wParam) == IDOK)
			{
			GetDlgItemText(hDlg, IDD_USER_NAME, TempString, 63);
			strcpy(szUsername,TempString);
			GetDlgItemText(hDlg, IDD_PASSWORD,TempString, 63);
			strcpy(szPassword,TempString);
			GetDlgItemText(hDlg, IDD_DOMAIN,TempString, 63);
			strcpy(szDomain,TempString);
			EndDialog(hDlg, 1);
			}
      return(TRUE);
    }

  return(FALSE);

}
int vncLogonThread::CreateLogonWindow(HINSTANCE hInstance)
{
	int returnvalue=0;

	// [v1.0.2-jp1 fix] Load resouce from dll
	//returnvalue = DialogBoxParam(hAppInstance,MAKEINTRESOURCE(IDD_LOGON),
	returnvalue = DialogBoxParam(hInstResDLL,MAKEINTRESOURCE(IDD_LOGON),
				NULL,
				(DLGPROC) LogonDlgProc,
				(LONG) this);

	if (returnvalue==1)
			
				if ((strcmp(szUsername ,TEXT(""))!=0) && (strcmp(szPassword ,TEXT(""))!=0))
					
						
						if (SSPLogonUser(szDomain, szUsername,szPassword)) return 3;
						
						
					
	
		
	return 2;
}
VOID vncLogonThread::CenterWindow(HWND hwnd)
{
  LONG    dx, dy;
  LONG    dxParent, dyParent;
  LONG    Style;
  logonhwnd=hwnd;
  GetWindowRect(hwnd, &rect);
  dx = rect.right - rect.left;
  dy = rect.bottom - rect.top;

  Style = GetWindowLong(hwnd, GWL_STYLE);
  if ((Style & WS_CHILD) == 0) 
    {
      dxParent = GetSystemMetrics(SM_CXSCREEN);
      dyParent = GetSystemMetrics(SM_CYSCREEN);
    } 
  else 
    {
      HWND    hwndParent;
      RECT    rectParent;

      hwndParent = GetParent(hwnd);
      if (hwndParent == NULL) 
	{
	  hwndParent = GetDesktopWindow();
	}

      GetWindowRect(hwndParent, &rectParent);

      dxParent = rectParent.right - rectParent.left;
      dyParent = rectParent.bottom - rectParent.top;
    }

  rect.left = (dxParent - dx) / 2+m_desktop->m_ScreenOffsetx;
  rect.top  = (dyParent - dy) / 3+m_desktop->m_ScreenOffsety;

  SetWindowPos(hwnd, HWND_TOPMOST, rect.left, rect.top, 0, 0, SWP_NOSIZE);

  SetForegroundWindow(hwnd);
}

