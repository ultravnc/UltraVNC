//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
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
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// vncSetAuth.cpp

// Implementation of the About dialog!
#include "stdafx.h"
#include "vncsetAuth.h"
#include "resource.h"
#define MAXSTRING 254
extern HINSTANCE hInst;




LONG
vncSetAuth::LoadInt_(LPCSTR valname, LONG defval)
{
	{
		return myIniFile.ReadInt("admin_auth", (char *)valname, defval);
	}
}

TCHAR *
vncSetAuth::LoadString_(LPCSTR keyname)
{
	{
		TCHAR *authhosts=new char[150];
		myIniFile.ReadString("admin_auth", (char *)keyname,authhosts,150);
		return (TCHAR *)authhosts;
	}
}

void
vncSetAuth::SaveInt_(LPCSTR valname, LONG val)
{
	{
		myIniFile.WriteInt("admin_auth", (char *)valname, val);
	}
}

void
vncSetAuth::SaveString_(LPCSTR valname, TCHAR *buffer)
{
	{
		myIniFile.WriteString("admin_auth", (char *)valname,buffer);
	}
}

void
vncSetAuth::savegroup1(TCHAR *value)
{
	{
		SaveString_("group1", value);
	}
}
TCHAR*
vncSetAuth::Readgroup1()
{
	{
		TCHAR *value=NULL;
		value=LoadString_("group1");
		return value;
	}
}

void
vncSetAuth::savegroup2(TCHAR *value)
{
	{
		SaveString_("group2", value);
	}
}
TCHAR*
vncSetAuth::Readgroup2()
{
	{
		TCHAR *value=NULL;
		value=LoadString_("group2");
		return value;
	}
}

void
vncSetAuth::savegroup3(TCHAR *value)
{
	{
		SaveString_("group3", value);
	}
}
TCHAR*
vncSetAuth::Readgroup3()
{
	{
		TCHAR *value=NULL;
		value=LoadString_("group3");
		return value;
	}
}

LONG
vncSetAuth::Readlocdom1(LONG returnvalue)
{

	{
		returnvalue=LoadInt_("locdom1",returnvalue);
		return returnvalue;
	}
}

void
vncSetAuth::savelocdom1(LONG value)
{

	{
		SaveInt_("locdom1", value);
	}

}

LONG
vncSetAuth::Readlocdom2(LONG returnvalue)
{

	{
		returnvalue=LoadInt_("locdom2",returnvalue);
		return returnvalue;
	}
}

void
vncSetAuth::savelocdom2(LONG value)
{

	{
		SaveInt_("locdom2", value);
	}

}

LONG
vncSetAuth::Readlocdom3(LONG returnvalue)
{

	{
		returnvalue=LoadInt_("locdom3",returnvalue);
		return returnvalue;
	}
}

void
vncSetAuth::savelocdom3(LONG value)
{
	{
		SaveInt_("locdom3", value);
	}

}

///////////////////////////////////////////////////////////



// Constructor/destructor
vncSetAuth::vncSetAuth()
{
	m_fUseRegistry = ((myIniFile.ReadInt("admin", "UseRegistry", 0) == 1) ? TRUE : FALSE);
	m_dlgvisible = FALSE;
	locdom1=0;
	locdom2=0;
	locdom3=0;
	group1=Readgroup1();
	group2=Readgroup2();
	group3=Readgroup3();
	locdom1=Readlocdom1(locdom1);
	locdom2=Readlocdom2(locdom2);
	locdom3=Readlocdom3(locdom3);
	if (group1){strcpy(pszgroup1,group1);delete [] group1;}
	else strcpy(pszgroup1,"");
	if (group2){strcpy(pszgroup2,group2);delete [] group2;}
	else strcpy(pszgroup2,"");
	if (group3){strcpy(pszgroup3,group3);delete [] group3;}
	else strcpy(pszgroup3,"");
}

vncSetAuth::~vncSetAuth()
{
}

// Initialisation
BOOL
vncSetAuth::Init()
{
	m_dlgvisible = FALSE;
	locdom1=0;
	locdom2=0;
	locdom3=0;
	group1=Readgroup1();
	group2=Readgroup2();
	group3=Readgroup3();
	locdom1=Readlocdom1(locdom1);
	locdom2=Readlocdom2(locdom2);
	locdom3=Readlocdom3(locdom3);
	if (group1){strcpy(pszgroup1,group1);delete group1;}
	else strcpy(pszgroup1,"");
	if (group2){strcpy(pszgroup2,group2);delete group2;}
	else strcpy(pszgroup2,"");
	if (group3){strcpy(pszgroup3,group3);delete group3;}
	else strcpy(pszgroup3,"");
	return TRUE;
}

// Dialog box handling functions
void
vncSetAuth::Show(BOOL show)
{
	if (show)
	{
		if (!m_dlgvisible)
		{
			// [v1.0.2-jp1 fix] Load resouce from dll
			//DialogBoxParam(hAppInstance,
			DialogBoxParam(hInst,
				MAKEINTRESOURCE(IDD_MSLOGON), 
				NULL,
				(DLGPROC) DialogProc,
				(LONG) this);
		}
	}
}

BOOL CALLBACK
vncSetAuth::DialogProc(HWND hwnd,
					 UINT uMsg,
					 WPARAM wParam,
					 LPARAM lParam )
{
	// We use the dialog-box's USERDATA to store a _this pointer
	// This is set only once WM_INITDIALOG has been recieved, though!
#ifndef _X64
	vncSetAuth *_this = (vncSetAuth *) GetWindowLong(hwnd, GWL_USERDATA);
#else
	vncSetAuth *_this = (vncSetAuth *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
#endif

	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			// Retrieve the Dialog box parameter and use it as a pointer
			// to the calling vncProperties object
			#ifndef _X64
				SetWindowLong(hwnd, GWL_USERDATA, lParam);
			#else
				SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
			#endif

			_this = (vncSetAuth *) lParam;
			SetDlgItemText(hwnd, IDC_GROUP1, _this->pszgroup1);
			SetDlgItemText(hwnd, IDC_GROUP2, _this->pszgroup2);
			SetDlgItemText(hwnd, IDC_GROUP3, _this->pszgroup3);
			if (_this->locdom1==1 || _this->locdom1==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG1L);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG1L);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (_this->locdom1==2 || _this->locdom1==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG1D);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG1D);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (_this->locdom2==1 || _this->locdom2==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG2L);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG2L);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (_this->locdom2==2 || _this->locdom2==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG2D);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG2D);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (_this->locdom3==1 || _this->locdom3==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG3L);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG3L);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (_this->locdom3==2 || _this->locdom3==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG3D);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG3D);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			
			// Show the dialog
			SetForegroundWindow(hwnd);

			_this->m_dlgvisible = TRUE;

			return TRUE;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{

		case IDCANCEL:
			EndDialog(hwnd, TRUE);
				_this->m_dlgvisible = FALSE;
				return TRUE;
		case IDOK:
			{
				_this->locdom1=0;
				_this->locdom2=0;
				_this->locdom3=0;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG1L),BM_GETCHECK,0,0) == BST_CHECKED) _this->locdom1=_this->locdom1+1;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG1D),BM_GETCHECK,0,0) == BST_CHECKED) _this->locdom1=_this->locdom1+2;

				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG2L),BM_GETCHECK,0,0) == BST_CHECKED) _this->locdom2=_this->locdom2+1;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG2D),BM_GETCHECK,0,0) == BST_CHECKED) _this->locdom2=_this->locdom2+2;

				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG3L),BM_GETCHECK,0,0) == BST_CHECKED) _this->locdom3=_this->locdom3+1;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG3D),BM_GETCHECK,0,0) == BST_CHECKED) _this->locdom3=_this->locdom3+2;

				GetDlgItemText(hwnd, IDC_GROUP1, (LPSTR) _this->pszgroup1, 240);
				GetDlgItemText(hwnd, IDC_GROUP2, (LPSTR) _this->pszgroup2, 240);
				GetDlgItemText(hwnd, IDC_GROUP3, (LPSTR) _this->pszgroup3, 240);
	
				_this->savegroup1(_this->pszgroup1);
				_this->savegroup2(_this->pszgroup2);
				_this->savegroup3(_this->pszgroup3);
				_this->savelocdom1(_this->locdom1);
				_this->savelocdom2(_this->locdom2);
				_this->savelocdom3(_this->locdom3);
				EndDialog(hwnd, TRUE);
				_this->m_dlgvisible = FALSE;
				return TRUE;
			}
		}

		break;

	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		_this->m_dlgvisible = FALSE;
		return TRUE;
	}
	return 0;
}
