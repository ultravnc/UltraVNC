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
	void updateUser(HWND hwnd);
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
	int authClientCount = -1;
	static char exe_file_name[MAX_PATH];
};


#endif