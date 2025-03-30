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


#if (!defined(_HELPERFUNCTIONS))
#define _HELPERFUNCTIONS

#pragma once

#include <tlhelp32.h>
#include <string>

#define MSGFLT_ADD		1
extern bool ClientTimerReconnect;

extern HWND G_MENU_HWND;
extern char* MENU_CLASS_NAME;
extern bool allowMultipleInstances;

void Open_homepage();
void Open_forum();
void Open_github();
void Open_mastodon();
void Open_bluesky();
void Open_facebook();
void Open_xtwitter();
void Open_reddit();
void Open_openhub();

namespace postHelper {
	extern UINT MENU_ADD_CLIENT_MSG;
	extern UINT MENU_REPEATER_ID_MSG;
	extern UINT MENU_AUTO_RECONNECT_MSG;
	extern UINT MENU_STOP_RECONNECT_MSG;
	extern UINT MENU_STOP_ALL_RECONNECT_MSG;
	extern UINT MENU_ADD_CLIENT_MSG_INIT;
	extern UINT MENU_ADD_CLOUD_MSG;
	extern UINT MENU_ADD_CLIENT6_MSG_INIT;
	extern UINT MENU_ADD_CLIENT6_MSG;
	extern UINT MENU_TRAYICON_BALLOON_MSG;
	extern UINT FileTransferSendPacketMessage;
	extern in6_addr G_LPARAM_IN6;

	BOOL PostAddAutoConnectClient(const char* pszId);
	BOOL PostAddNewRepeaterClient();
	BOOL PostAddNewCloudClient();
	BOOL PostAddStopConnectClient();
	BOOL PostAddStopConnectClientAll();
	BOOL PostAddNewClientInit(unsigned long ipaddress, unsigned short port);

	BOOL PostAddNewClient4(unsigned long ipaddress, unsigned short port);
	BOOL PostAddNewClientInit4(unsigned long ipaddress, unsigned short port);
	BOOL PostAddNewClient6(in6_addr* ipaddress, unsigned short port);
	BOOL PostAddNewClientInit6(in6_addr* ipaddress, unsigned short port);

	BOOL PostAddNewClient(unsigned long ipaddress, unsigned short port);
	BOOL PostAddConnectClient(const char* pszId);
	BOOL PostToThisWinVNC(UINT message, WPARAM wParam, LPARAM lParam);
	BOOL PostToWinVNC(UINT message, WPARAM wParam, LPARAM lParam);

	HWND FindWinVNCWindow(bool bThisProcess);
}

namespace processHelper {
	BOOL CurrentUser(char* buffer, UINT size);
	DWORD GetExplorerLogonPid();
	bool IsServiceInstalled();
	bool IsServiceRunning();
	DWORD GetCurrentConsoleSessionID();
	BOOL IsWSLocked();
}

namespace desktopSelector {
	int InputDesktopSelected();
	BOOL SelectHDESK(HDESK new_desktop);
	BOOL SelectDesktop(char* name, HDESK* new_desktop);
}

#ifndef SC_20
namespace serviceHelpers {
	//bool ExistServiceName(TCHAR* pszAppPath, TCHAR* pszServiceName);
	void make_upper(std::string& str);
	void winvncSecurityEditorHelper_as_admin();
	void Set_uninstall_service_as_admin();
	void Set_install_service_as_admin();
	void Real_start_service();
	void Set_start_service_as_admin();
	void Real_stop_service();
	void Set_stop_service_as_admin();
}
#endif // SC_20

DWORD MessageBoxSecure(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);

#endif