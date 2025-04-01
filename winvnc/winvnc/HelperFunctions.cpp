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


#include <algorithm>
#include "stdhdrs.h"
#include "common/inifile.h"
#include <cctype>
#include <cassert>
#include "UltraVNCService.h"
#include <winvnc/winvnc.h>
#include "SettingsManager.h"
#include <lmcons.h>
#include "credentials.h"
#include "common/win32_helpers.h"
//  We first use shellexecute with "runas"
//  This way we can use UAC and user/passwd
//	Runas is standard OS, so no security risk

HWND G_MENU_HWND = NULL;
char* MENU_CLASS_NAME = "WinVNC Tray Icon";
bool ClientTimerReconnect = false;
bool allowMultipleInstances = false;
extern HINSTANCE	hInstResDLL;

void Open_homepage()
{
	ShellExecute(0, "open", "https://uvnc.com/", 0, 0, 1);
}

void Open_forum()
{
	ShellExecute(0, "open", "https://forum.uvnc.com/", 0, 0, 1);
}

void Open_github()
{
	ShellExecute(0, "open", "https://github.com/ultravnc", 0, 0, 1);
}

void Open_mastodon()
{
	ShellExecute(0, "open", "https://mastodon.social/@ultravnc", 0, 0, 1);
}

void Open_bluesky()
{
	ShellExecute(0, "open", "https://bsky.app/profile/ultravnc.bsky.social", 0, 0, 1);
}

void Open_facebook()
{
	ShellExecute(0, "open", "https://www.facebook.com/ultravnc1", 0, 0, 1);
}

void Open_xtwitter()
{
	ShellExecute(0, "open", "https://x.com/ultravnc1", 0, 0, 1);
}

void Open_reddit()
{
	ShellExecute(0, "open", "https://www.reddit.com/r/ultravnc", 0, 0, 1);
}

void Open_openhub()
{
	ShellExecute(0, "open", "https://openhub.net/p/ultravnc", 0, 0, 1);
}

#ifndef SC_20
namespace serviceHelpers {
	void Set_stop_service_as_admin() {
		char exe_file_name[MAX_PATH];
		GetModuleFileName(0, exe_file_name, MAX_PATH);

		SHELLEXECUTEINFO shExecInfo{};

		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = NULL;
		shExecInfo.hwnd = GetForegroundWindow();
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = exe_file_name;
		shExecInfo.lpParameters = winvncStopservice;
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = NULL;
		ShellExecuteEx(&shExecInfo);
	}

	void Real_stop_service() {
		char command[MAX_PATH + 32]; // 29 January 2008 jdp
		_snprintf_s(command, sizeof command, "net stop \"%s\"", UltraVNCService::service_name);
		WinExec(command, SW_HIDE);
	}

	void Set_start_service_as_admin() {
		char exe_file_name[MAX_PATH];
		GetModuleFileName(0, exe_file_name, MAX_PATH);

		SHELLEXECUTEINFO shExecInfo{};

		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = NULL;
		shExecInfo.hwnd = GetForegroundWindow();
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = exe_file_name;
		shExecInfo.lpParameters = winvncStartservice;
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = NULL;
		ShellExecuteEx(&shExecInfo);
	}

	void Real_start_service() {
		char command[MAX_PATH + 32]; // 29 January 2008 jdp
		_snprintf_s(command, sizeof command, "net start \"%s\"", UltraVNCService::service_name);
		WinExec(command, SW_HIDE);
	}

	void Set_install_service_as_admin() {
		char exe_file_name[MAX_PATH];
		GetModuleFileName(0, exe_file_name, MAX_PATH);

		SHELLEXECUTEINFO shExecInfo{};

		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = NULL;
		shExecInfo.hwnd = GetForegroundWindow();
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = exe_file_name;
		shExecInfo.lpParameters = winvncInstallService;
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = NULL;
		ShellExecuteEx(&shExecInfo);
	}

	void Set_uninstall_service_as_admin() {
		char exe_file_name[MAX_PATH];
		GetModuleFileName(0, exe_file_name, MAX_PATH);

		SHELLEXECUTEINFO shExecInfo{};

		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = NULL;
		shExecInfo.hwnd = GetForegroundWindow();
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = exe_file_name;
		shExecInfo.lpParameters = winvncUnInstallService;
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = NULL;
		ShellExecuteEx(&shExecInfo);
	}

	void winvncSecurityEditorHelper_as_admin() {
		char exe_file_name[MAX_PATH];
		GetModuleFileName(0, exe_file_name, MAX_PATH);

		SHELLEXECUTEINFO shExecInfo{};

		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = NULL;
		shExecInfo.hwnd = GetForegroundWindow();
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = exe_file_name;
		shExecInfo.lpParameters = winvncSecurityEditor;
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = NULL;
		ShellExecuteEx(&shExecInfo);
	}

	void make_upper(std::string& str)
	{
		// convert to uppercase
		std::transform(str.begin(), str.end(), str.begin(), toupper);//(int(*)(int))
	}

	//**************************************************************************
	// ExistServiceName() looks up service by application path. If found, the function
	// fills pszServiceName (must be at least 256+1 characters long).
	bool ExistServiceName(TCHAR* pszAppPath, TCHAR* pszServiceName)
	{
		// prepare given application path for matching against service list
		std::string appPath(pszAppPath);
		// convert to uppercase
		make_upper(appPath);

		// connect to serice control manager
		SC_HANDLE hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
		if (!hManager)
			return false;

		DWORD dwBufferSize = 0;
		DWORD dwCount = 0;
		DWORD dwPosition = 0;
		bool bResult = false;

		// call EnumServicesStatus() the first time to receive services array size
		BOOL bOK = EnumServicesStatus(
			hManager,
			SERVICE_WIN32,
			SERVICE_STATE_ALL,
			NULL,
			0,
			&dwBufferSize,
			&dwCount,
			&dwPosition);
		if (!bOK && GetLastError() == ERROR_MORE_DATA)
		{
			// allocate space per results from the first call
			ENUM_SERVICE_STATUS* pServices = (ENUM_SERVICE_STATUS*) new UCHAR[dwBufferSize];
			if (pServices)
			{
				// call EnumServicesStatus() the second time to actually get the services array
				bOK = EnumServicesStatus(
					hManager,
					SERVICE_WIN32,
					SERVICE_STATE_ALL,
					pServices,
					dwBufferSize,
					&dwBufferSize,
					&dwCount,
					&dwPosition);
				if (bOK)
				{
					// iterate through all services returned by EnumServicesStatus()
					for (DWORD i = 0; i < dwCount && !bResult; i++)
					{
						// open service
						SC_HANDLE hService = OpenService(hManager,
							pServices[i].lpServiceName,
							GENERIC_READ);
						if (!hService)
							break;

						// call QueryServiceConfig() the first time to receive buffer size
						bOK = QueryServiceConfig(
							hService,
							NULL,
							0,
							&dwBufferSize);
						if (!bOK && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
						{
							// allocate space per results from the first call
							QUERY_SERVICE_CONFIG* pServiceConfig = (QUERY_SERVICE_CONFIG*) new UCHAR[dwBufferSize];
							if (pServiceConfig)
							{
								// call EnumServicesStatus() the second time to actually get service config
								bOK = QueryServiceConfig(
									hService,
									pServiceConfig,
									dwBufferSize,
									&dwBufferSize);
								if (bOK)
								{
									// match given application name against executable path in the service config
									std::string servicePath(pServiceConfig->lpBinaryPathName);
									make_upper(servicePath);
									if (servicePath.find(appPath.c_str()) != -1)
									{
										bResult = true;
										strncpy_s(pszServiceName, 256, pServices[i].lpServiceName, 256);
										pszServiceName[255] = 0;
									}
								}

								delete[](UCHAR*) pServiceConfig;
							}
						}

						CloseServiceHandle(hService);
					}
				}

				delete[](UCHAR*) pServices;
			}
		}

		// disconnect from serice control manager
		CloseServiceHandle(hManager);

		return bResult;
	}
}
#endif // SC_20

DWORD MessageBoxSecure(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
	DWORD retunvalue = 0;
	if (settings->RunningFromExternalService())
	{
		HDESK desktop = NULL;
		HDESK old_desktop;
		desktop = OpenInputDesktop(0, FALSE, DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL | DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
		old_desktop = GetThreadDesktop(GetCurrentThreadId());
		if (desktop && old_desktop && old_desktop != desktop)
		{
			SetThreadDesktop(desktop);
			if (uType & MB_SERVICE_NOTIFICATION) {
				retunvalue = MessageBox(hWnd, lpText, lpCaption, uType);
			}
			else if (uType & MB_YESNO) {
				BOOL bCheckboxChecked;
				retunvalue = helper::yesnoUVNCMessageBox(hInstResDLL, hWnd, (char*)lpCaption, (char*)lpText, "YES", "NO", "", bCheckboxChecked);

			}
			else {
				if (uType & MB_OK)
					uType &= ~MB_OK;
				retunvalue = helper::yesUVNCMessageBox(hInstResDLL, hWnd, (char*)lpText, (char*)lpCaption, uType);
			}

			SetThreadDesktop(old_desktop);
			CloseDesktop(desktop);
		}
		else retunvalue = 0;
	}
	else
	{
		if (uType & MB_SERVICE_NOTIFICATION) {
			retunvalue = MessageBox(hWnd, lpText, lpCaption, uType);
		}
		else if (uType & MB_YESNO) {
			BOOL bCheckboxChecked;
			retunvalue = helper::yesnoUVNCMessageBox(hInstResDLL, hWnd, (char*)lpCaption, (char*)lpText, "YES", "NO", "", bCheckboxChecked);

		}
		else {
			if (uType & MB_OK)
				uType &= ~MB_OK;
			retunvalue = helper::yesUVNCMessageBox(hInstResDLL, hWnd, (char*)lpText, (char*)lpCaption, uType);
		}
	}
	return retunvalue;
}

namespace desktopSelector {
	int closeHandlesAndReturn(HDESK threaddesktop, HDESK inputdesktop, int value) {
		if(threaddesktop)
			CloseDesktop(threaddesktop);
		if (inputdesktop)
			CloseDesktop(inputdesktop);
		return value;
	}
	int InputDesktopSelected() {
		// Get the input and thread desktops
		HDESK threaddesktop = GetThreadDesktop(GetCurrentThreadId());
		HDESK inputdesktop = OpenInputDesktop(0, FALSE,
			DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
			DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
			DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
			DESKTOP_SWITCHDESKTOP);

		if (inputdesktop == NULL) {
			if (!settings->RunningFromExternalService())
				return closeHandlesAndReturn(threaddesktop, inputdesktop, 2); 			//Running as SC we want to keep the viewer open in case UAC or screensaver jump in
			DWORD lasterror;
			lasterror = GetLastError();
			if (lasterror == 170)
				return closeHandlesAndReturn(threaddesktop, inputdesktop, 1);
			if (lasterror == 624)
				return closeHandlesAndReturn(threaddesktop, inputdesktop, 1);
			return closeHandlesAndReturn(threaddesktop, inputdesktop, 0);
		}

		DWORD dummy;
		char threadname[256]{};
		char inputname[256]{};

		if (!GetUserObjectInformation(threaddesktop, UOI_NAME, &threadname, 256, &dummy)) {
			if (!settings->RunningFromExternalService())
				return closeHandlesAndReturn(threaddesktop, inputdesktop, 2);
			return closeHandlesAndReturn(threaddesktop, inputdesktop, 0);
		}
		assert(dummy <= 256);
		if (!GetUserObjectInformation(inputdesktop, UOI_NAME, &inputname, 256, &dummy)) {
			if (!settings->RunningFromExternalService())
				return closeHandlesAndReturn(threaddesktop, inputdesktop, 2);
			return closeHandlesAndReturn(threaddesktop, inputdesktop, 0);
		}
		assert(dummy <= 256);

		if (strcmp(threadname, inputname) != 0) {
			if (!settings->RunningFromExternalService())
				return closeHandlesAndReturn(threaddesktop, inputdesktop, 2);
			return closeHandlesAndReturn(threaddesktop, inputdesktop, 0);
		}
		return closeHandlesAndReturn(threaddesktop, inputdesktop, 1);
	}

	BOOL SelectHDESK(HDESK new_desktop) {
		HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());

		DWORD dummy;
		char new_name[256]{};

		if (!GetUserObjectInformation(new_desktop, UOI_NAME, &new_name, 256, &dummy)) {
			vnclog.Print(LL_INTERR, VNCLOG("!GetUserObjectInformation \n"));
			return FALSE;
		}

		vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK to %s (%x) from %x\n"), new_name, new_desktop, old_desktop);

		// Switch the desktop
		if (!SetThreadDesktop(new_desktop)) {
			vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK:!SetThreadDesktop \n"));
			return FALSE;
		}
		return TRUE;
	}

	BOOL SelectDesktop(char* name, HDESK* new_desktop) {
		HDESK desktop;
		vnclog.Print(LL_INTERR, VNCLOG("SelectDesktop \n"));
		if (name != NULL)
		{
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 named\n"));
			// Attempt to open the named desktop
			desktop = OpenDesktop(name, 0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
		}
		else
		{
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 NULL\n"));
			// No, so open the input desktop
			desktop = OpenInputDesktop(0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
		}

		// Did we succeed?
		if (desktop == NULL) {
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 \n"));
			return FALSE;
		}
		else vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 OK\n"));

		// Switch to the new desktop
		if (!SelectHDESK(desktop)) {
			// Failed to enter the new desktop, so free it!
			if (!CloseDesktop(desktop))
				vnclog.Print(LL_INTERR, VNCLOG("SelectDesktop failed to close desktop\n"));
			return FALSE;
		}

		if (new_desktop)
		{
			if (*new_desktop)
				CloseDesktop(*new_desktop);
			*new_desktop = desktop;
		}

		// We successfully switched desktops!
		return TRUE;
	}
}

namespace postHelper {
	UINT MENU_ADD_CLIENT_MSG = RegisterWindowMessage("WinVNC.AddClient.Message");
	UINT MENU_ADD_CLOUD_MSG = RegisterWindowMessage("WinVNC.AddCloud.Message");
	UINT MENU_REPEATER_ID_MSG = RegisterWindowMessage("WinVNC.AddRepeaterID.Message");
	UINT MENU_AUTO_RECONNECT_MSG = RegisterWindowMessage("WinVNC.AddAutoClient.Message");
	UINT MENU_STOP_RECONNECT_MSG = RegisterWindowMessage("WinVNC.AddStopClient.Message");
	UINT MENU_STOP_ALL_RECONNECT_MSG = RegisterWindowMessage("WinVNC.AddStopAllClient.Message");
	UINT MENU_ADD_CLIENT_MSG_INIT = RegisterWindowMessage("WinVNC.AddClient.Message.Init");
	UINT MENU_ADD_CLIENT6_MSG_INIT = RegisterWindowMessage("WinVNC.AddClient6.Message.Init");
	UINT MENU_ADD_CLIENT6_MSG = RegisterWindowMessage("WinVNC.AddClient6.Message");
	UINT MENU_TRAYICON_BALLOON_MSG = RegisterWindowMessage("WinVNC.TrayIconBalloon2.Message");
	UINT FileTransferSendPacketMessage = RegisterWindowMessage("UltraVNC.Viewer.FileTransferSendPacketMessage");

	in6_addr G_LPARAM_IN6 = {};

	BOOL PostAddAutoConnectClient(const char* pszId) {
		ATOM aId = INVALID_ATOM;
		if (pszId)
			aId = GlobalAddAtom(pszId);
		return (PostToWinVNC(MENU_AUTO_RECONNECT_MSG, 0, (LPARAM)aId));
	}

	BOOL PostAddNewRepeaterClient() {
		// assumes the -repeater command line set the repeater global variable.
		// Post to the UltraVNC Server menu window (usually expected to fail at program startup)
		if (!PostToWinVNC(MENU_ADD_CLIENT_MSG, (WPARAM)0xFFFFFFFF, (LPARAM)0xFFFFFFFF))
			return FALSE;
		return TRUE;
	}

	BOOL PostAddNewCloudClient() {
		// assumes the -repeater command line set the repeater global variable.
		// Post to the UltraVNC Server menu window (usually expected to fail at program startup)
		if (!PostToWinVNC(MENU_ADD_CLOUD_MSG, (WPARAM)0xFFFFFFFF, (LPARAM)0xFFFFFFFF))
			return FALSE;
		return TRUE;
	}

	BOOL PostAddStopConnectClient() {
		return (PostToWinVNC(MENU_STOP_RECONNECT_MSG, 0, 0));
	}

	BOOL PostAddStopConnectClientAll() {
		PostToWinVNC(MENU_STOP_RECONNECT_MSG, 0, 0); // stop running reconnect in server class
		return (PostToWinVNC(MENU_STOP_ALL_RECONNECT_MSG, 0, 0)); // disable reconnect for tunning clients
	}

	BOOL PostAddNewClientInit(unsigned long ipaddress, unsigned short port) {
		// Post to the UltraVNC Server menu window
		if (!PostToWinVNC(MENU_ADD_CLIENT_MSG_INIT, (WPARAM)port, (LPARAM)ipaddress))
		{
			vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient failed\n"));
			if (port == 1111 && ipaddress == 1111) ClientTimerReconnect = true;
			return FALSE;
		}

		return TRUE;
	}

	BOOL PostAddNewClient4(unsigned long ipaddress, unsigned short port) {
		// Post to the UltraVNC Server menu window
		if (!PostToWinVNC(MENU_ADD_CLIENT_MSG, (WPARAM)port, (LPARAM)ipaddress))
		{
			vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient failed\n"));
			if (port == 1111 && ipaddress == 1111) ClientTimerReconnect = true;
			return FALSE;
		}

		return TRUE;
	}

	BOOL PostAddNewClientInit4(unsigned long ipaddress, unsigned short port) {
		// Post to the UltraVNC Server menu window
		if (!PostToWinVNC(MENU_ADD_CLIENT_MSG_INIT, (WPARAM)port, (LPARAM)ipaddress)) {
			vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient failed\n"));
			if (port == 1111 && ipaddress == 1111) ClientTimerReconnect = true;
			return FALSE;
		}
		return TRUE;
	}
	BOOL PostAddNewClient6(in6_addr* ipaddress, unsigned short port) {
		// Post to the UltraVNC Server menu window
		// We can not sen a IPv6 address with a LPARAM, so we fake the message by copying in a gloobal var.
		memcpy(&G_LPARAM_IN6, ipaddress, sizeof(in6_addr));
		if (!PostToWinVNC(MENU_ADD_CLIENT6_MSG, (WPARAM)port, (LPARAM)ipaddress))
			return FALSE;
		return TRUE;
	}

	BOOL PostAddNewClientInit6(in6_addr* ipaddress, unsigned short port) {
		// Post to the UltraVNC Server menu window
		// We can not sen a IPv6 address with a LPARAM, so we fake the message by copying in a gloobal var.
		memcpy(&G_LPARAM_IN6, ipaddress, sizeof(in6_addr));
		if (!PostToWinVNC(MENU_ADD_CLIENT6_MSG_INIT, (WPARAM)port, (LPARAM)ipaddress))
			return FALSE;
		return TRUE;
	}

	BOOL PostAddNewClient(unsigned long ipaddress, unsigned short port) {
		// Post to the UltraVNC Server menu window
		if (!PostToWinVNC(MENU_ADD_CLIENT_MSG, (WPARAM)port, (LPARAM)ipaddress))
		{
			vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient failed\n"));
			if (port == 1111 && ipaddress == 1111) ClientTimerReconnect = true;
			return FALSE;
		}
		return TRUE;
	}

	BOOL PostAddConnectClient(const char* pszId) {
		ATOM aId = INVALID_ATOM;
		if (pszId) {
			aId = GlobalAddAtom(pszId);
		}
		return (PostToWinVNC(MENU_REPEATER_ID_MSG, 0, (LPARAM)aId));
	}

	BOOL PostToThisWinVNC(UINT message, WPARAM wParam, LPARAM lParam) {
		//adzm 2010-02-10 - Finds the appropriate VNC window for this process
		HWND hservwnd = FindWinVNCWindow(true);
		if (hservwnd == NULL)
			return FALSE;

		// Post the message to UltraVNC Server
		PostMessage(hservwnd, message, wParam, lParam);
		return TRUE;
	}

	BOOL PostToWinVNC(UINT message, WPARAM wParam, LPARAM lParam) {
		// Locate the hidden UltraVNC Server menu window
		// adzm 2010-02-10 - If we are in SC mode, then we know we want to only post messages to our own instance. This prevents
		// conflicts if the user already has another copy of a UltraVNC Server derived application running.
		if (allowMultipleInstances || settings->getScExit() || settings->getScPrompt()) {
			return PostToThisWinVNC(message, wParam, lParam);
		}

		//adzm 2010-02-10 - Finds the appropriate VNC window
		HWND hservwnd = FindWinVNCWindow(false);
		if (hservwnd == NULL)
			return FALSE;

		// Post the message to UltraVNC Server
		PostMessage(hservwnd, message, wParam, lParam);
		return TRUE;
	}

	HWND FindWinVNCWindow(bool bThisProcess) {
		// Locate the hidden UltraVNC Server menu window
		if (!bThisProcess) {
			// Find any window with the MENU_CLASS_NAME window class
			HWND returnvalue = FindWindow(MENU_CLASS_NAME, NULL);
			if (returnvalue == NULL) goto nullreturn;
			return returnvalue;
		}
		else {
			// Find one that matches the class and is the same process
			HWND hwndZ = NULL;
			HWND hwndServer = NULL;
			while (!hwndServer) {
				hwndServer = FindWindowEx(NULL, hwndZ, MENU_CLASS_NAME, NULL);

				if (hwndServer != NULL) {
					DWORD dwProcessId = 0;
					GetWindowThreadProcessId(hwndServer, &dwProcessId);

					if (dwProcessId == GetCurrentProcessId()) {
						return hwndServer;
					}
					else {
						hwndZ = hwndServer;
						hwndServer = NULL;
					}
				}
				else {
					goto nullreturn;
				}
			}
		}

	nullreturn:
		return G_MENU_HWND;
	}
}

namespace processHelper {
	DWORD GetExplorerLogonPid()
	{
		char alternate_shell[129];
		strcpy_s(alternate_shell, "");
		strcpy_s(alternate_shell, settings->getAlternateShell());
		DWORD dwSessionId;
		DWORD dwExplorerLogonPid = 0;
		PROCESSENTRY32 procEntry{};
		dwSessionId = WTSGetActiveConsoleSessionId();
		if (GetSystemMetrics(SM_REMOTESESSION)) {
			DWORD dw = GetCurrentProcessId();
			DWORD pSessionId = 0xFFFFFFFF;
			ProcessIdToSessionId(dw, &pSessionId);
			dwSessionId = pSessionId;
		}

		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnap == INVALID_HANDLE_VALUE) {
			return 0;
		}
		procEntry.dwSize = sizeof(PROCESSENTRY32);
		if (!Process32First(hSnap, &procEntry)) {
			CloseHandle(hSnap);
			return 0;
		}

		do {
			if ((_stricmp(procEntry.szExeFile, "explorer.exe") == 0) || (strlen(alternate_shell) != 0 && (_stricmp(procEntry.szExeFile, alternate_shell) == 0))) {
				DWORD dwExplorerSessId = 0;
				if (ProcessIdToSessionId(procEntry.th32ProcessID, &dwExplorerSessId)
					&& dwExplorerSessId == dwSessionId) {
					dwExplorerLogonPid = procEntry.th32ProcessID;
					break;
				}
			}
		} while (Process32Next(hSnap, &procEntry));
		CloseHandle(hSnap);
		return dwExplorerLogonPid;
	}

	bool GetConsoleUser(char* buffer, UINT size)
	{
		HANDLE hPToken = DesktopUsersToken::getInstance()->getDesktopUsersToken();
		if (hPToken == NULL) {
			strcpy_s(buffer, UNLEN + 1, "");
			return 0;
		}

		char aa[16384]{};
		// token user
		TOKEN_USER* ptu;
		DWORD needed;
		ptu = (TOKEN_USER*)aa;//malloc( 16384 );
		if (GetTokenInformation(hPToken, TokenUser, ptu, 16384, &needed))
		{
			char  DomainName[64];
			memset(DomainName, 0, sizeof(DomainName));
			DWORD DomainSize;
			DomainSize = sizeof(DomainName) - 1;
			SID_NAME_USE SidType;
			DWORD dwsize = size;
			LookupAccountSid(NULL, ptu->User.Sid, buffer, &dwsize, DomainName, &DomainSize, &SidType);
			//free(ptu);
			return 1;
		}
		//free(ptu);
		strcpy_s(buffer, UNLEN + 1, "");
		return 0;
	}

	BOOL GetCurrentUser(char* buffer, UINT size) // RealVNC 336 change
	{
		BOOL	g_impersonating_user = FALSE;
		// How to obtain the name of the current user depends upon the OS being used
		if (settings->RunningFromExternalService()) {
			// Get the current Window station
			g_impersonating_user = TRUE;

			HWINSTA station = GetProcessWindowStation();
			if (station == NULL)
				return FALSE;

			DWORD usersize;
			GetUserObjectInformation(station, UOI_USER_SID, NULL, 0, &usersize);

			// Check the required buffer size isn't zero
			if (usersize == 0) {
				// No user is logged in - ensure we're not impersonating anyone
				RevertToSelf();
				g_impersonating_user = FALSE;
				if (strlen("") >= size)
					return FALSE;
				strcpy_s(buffer, UNLEN + 1, "");
				return TRUE;
			}

			if (!g_impersonating_user) {
				if (strlen("") >= size)
					return FALSE;
				strcpy_s(buffer, UNLEN + 1, "");
				return TRUE;
			}
		}

		DWORD length = size;
		if (GetConsoleUser(buffer, size) == 0) {
			if (GetUserName(buffer, &length) == 0) {
				UINT error = GetLastError();
				if (error == ERROR_NOT_LOGGED_ON) {
					// No user logged on
					if (strlen("") >= size)
						return FALSE;
					strcpy_s(buffer, UNLEN + 1, "");
					return TRUE;
				}
				else {
					// Genuine error...
					vnclog.Print(LL_INTERR, VNCLOG("GetUsername error %d\n"), GetLastError());
					return FALSE;
				}
			}
		}
		return TRUE;
	}

	BOOL CurrentUser(char* buffer, UINT size)
	{
		// Attempt to get the current user
		BOOL result = GetCurrentUser(buffer, size);

		// If no user is logged in, and we're not running as an external service
		if (result && (strcmp(buffer, "") == 0) && !settings->RunningFromExternalService()) {
			// Ensure buffer has enough space for "Default" + null terminator
			if (size >= sizeof("Default")) {
				strncpy_s(buffer, size, "Default", _TRUNCATE);  // Set buffer to "Default" safely
			}
		}

		return result;
	}
#ifndef SC_20
	bool IsServiceRunning() {
		bool isRunning = false;

		// Open the Service Control Manager
		SC_HANDLE hSCM = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
		if (!hSCM) {
			return false;
		}

		// Open the service
		SC_HANDLE hService = OpenService(hSCM, UltraVNCService::service_name, SERVICE_QUERY_STATUS);
		if (!hService) {
			CloseServiceHandle(hSCM);
			return false;
		}

		// Query the service status
		SERVICE_STATUS_PROCESS serviceStatus;
		DWORD bytesNeeded;
		if (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO,
			(LPBYTE)&serviceStatus, sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded)) {
			if (serviceStatus.dwCurrentState == SERVICE_RUNNING) {
				isRunning = true;
			}
		}

		// Cleanup
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCM);

		return isRunning;
	}
#endif
	bool IsServiceInstalled()
	{
		bool serviceInstalled = false;

#ifndef SC_20
		// Open the Service Control Manager with permission to enumerate services
		SC_HANDLE hSCM = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
		if (hSCM) {
			// Attempt to open the specified service with query configuration access
			SC_HANDLE hService = ::OpenService(hSCM, UltraVNCService::service_name, SERVICE_QUERY_CONFIG);
			if (hService) {
				serviceInstalled = true;
				::CloseServiceHandle(hService);
			}
			::CloseServiceHandle(hSCM);
		}
#endif // SC_20

		return serviceInstalled;
	}

	DWORD GetCurrentConsoleSessionID()
	{
		return WTSGetActiveConsoleSessionId();
	}

	BOOL IsWSLocked()
	{
		bool bLocked = false;

		// Original code does not work if running as a service... apparently no access to the desktop.
		// Alternative is to check for a running LogonUI.exe (if present, system is either not logged in or locked)
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		PROCESSENTRY32W procentry{};
		procentry.dwSize = sizeof(procentry);

		if (Process32FirstW(hSnap, &procentry)) {
			do {
				if (!_wcsicmp(procentry.szExeFile, L"LogonUI.exe")) {
					bLocked = true;
					break;
				}
			} while (Process32NextW(hSnap, &procentry));
		}
		return bLocked;
	}
}
