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


#pragma once

#include "stdhdrs.h"
// Phil Money @ Advantig, LLC 7-9-2005
static bool ListenMode = true; 
void SetListenMode(bool listenmode); 
bool GetListenMode(); 


class Daemon  
{
public:
	Daemon(int port);
	virtual ~Daemon();
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
protected:
	void AddTrayIcon();
	void CheckTrayIcon();
	void RemoveTrayIcon();
	bool SendTrayMsg(DWORD msg);
#ifdef IPV6V4
	SOCKET m_deamon_sock6;
	SOCKET m_deamon_sock4;
#else
	SOCKET m_deamon_sock;
#endif

	HWND m_hwnd;
	HMENU m_hmenu;
	UINT m_timer;
	NOTIFYICONDATA m_nid;
	int m_nPort; // sf@2003
	char netbuf[1024];
};

