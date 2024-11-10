/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
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


// vncMenu

// This class handles creation of a system-tray icon & menu

class vncMenu;

#if (!defined(_WINVNC_VNCMENU))
#define _WINVNC_VNCMENU

#include "stdhdrs.h"
#include <lmcons.h>
#include "vncserver.h"
#include "PropertiesDialog.h"
#include "vncabout.h"
#include "vncListDlg.h"
#include "CloudDialog.h"

// The tray menu class itself
class vncMenu
{
public:
	vncMenu(vncServer *server);
	~vncMenu();

	void Shutdown(bool kill_client); // sf@2007

	// adzm 2009-07-05 - Tray icon balloon tips
	static void NotifyBalloon(wchar_t* szInfo, wchar_t* szTitle = NULL);
	static void updateList();
	static void updateMenu();
	static HMENU m_hmenu;

protected:
	// Tray icon handling
	void AddTrayIcon();
	void DelTrayIcon();
	void FlashTrayIcon(BOOL flash);
	void SendTrayMsg(DWORD msg, bool balloon, BOOL flash);
	void GetIPAddrString(char *buffer, int buflen);

	BOOL AddNotificationIcon();
	BOOL DeleteNotificationIcon();
	void setToolTip();
	void setBalloonInfo();
	void addMenus();
	void RestoreTooltip();

	// Message handler for the tray window
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static bool OpenWebpageFromApp(int iMsg);
	static bool OpenWebpageFromService(char* cmdline);

	// Fields
protected:
	// Check that the password has been set
	void CheckPassword();

	// The server that this Tray icon handles
	vncServer		*m_server;

	// Properties object for this server
	PropertiesDialog	m_properties;

	// About dialog for this server
	vncAbout		m_about;

	// List of viewers
	vncListDlg		m_ListDlg;

	HWND			m_hwnd;

	NOTIFYICONDATAW	m_nid{};
	omni_mutex		m_mutexTrayIcon; // adzm 2009-07-05
	wchar_t*			m_BalloonInfo;
	wchar_t*			m_BalloonTitle;

	char			m_username[UNLEN+1];

	// The icon handles
	HICON			m_winvnc_icon;
	HICON			m_flash_icon;

	BOOL bConnectSock;
	BOOL bAutoPort;
	UINT port_rfb;
	UINT port_http;
	BOOL ports_set;

	bool IsIconSet;
	int IconFaultCounter;
	bool balloonset = false;
	wchar_t m_tooltip[128]{};
};


#endif