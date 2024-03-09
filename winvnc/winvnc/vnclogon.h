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
#if !defined(_WINVNC_LOGON)
#define _WINVNC_LOGON
#pragma once
class vncLogonThread : public omni_thread
{
public:
	vncLogonThread() {
		m_returnsig = NULL;
		m_desktop = NULL;
		x1 = x2 = y1 = y2 = 0;
	}
	~vncLogonThread() {if (m_returnsig != NULL) delete m_returnsig;};
public:
	virtual BOOL Init(vncDesktop *desktop);
	virtual void *run_undetached(void *arg);
	HWND logonhwnd;
	RECT    rect;

protected:
	omni_mutex m_returnLock;
	omni_condition *m_returnsig;
	vncDesktop *m_desktop;
	int x1,x2,y1,y2;
	VOID CenterWindow(HWND hwnd);
	static BOOL CALLBACK LogonDlgProc(HWND hDlg,UINT Message,WPARAM wParam,LPARAM lParam);
	int CreateLogonWindow(HINSTANCE hInstance);
};
#endif