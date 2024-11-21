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


#pragma once
#define MAXSTRLENGTH    255

#include <initializer_list>

typedef struct _CPAU_PARAM {
	DWORD   cbSize;
	DWORD   dwProcessId;
	BOOL    bUseDefaultToken;
	HANDLE  hToken;
	LPWSTR  lpApplicationName;
	LPWSTR  lpCommandLine;
	SECURITY_ATTRIBUTES     ProcessAttributes;
	SECURITY_ATTRIBUTES ThreadAttributes;
	BOOL bInheritHandles;
	DWORD dwCreationFlags;
	LPVOID lpEnvironment;
	LPWSTR lpCurrentDirectory;
	STARTUPINFOW StartupInfo;
	PROCESS_INFORMATION     ProcessInformation;

}CPAU_PARAM;

typedef struct _CPAU_RET_PARAM {
	DWORD   cbSize;
	BOOL    bRetValue;
	DWORD   dwLastErr;
	PROCESS_INFORMATION     ProcInfo;

}CPAU_RET_PARAM;

typedef BOOLEAN(WINAPI* pWinStationQueryInformationW)(
	IN   HANDLE hServer,
	IN   ULONG LogonId,
	IN   DWORD /*WINSTATIONINFOCLASS*/ WinStationInformationClass,
	OUT  PVOID pWinStationInformation,
	IN   ULONG WinStationInformationLength,
	OUT  PULONG pReturnLength
	);

class UltraVNCService {
	
private:
	static void WINAPI service_main(DWORD argc, LPTSTR* argv);
	static void WINAPI control_handler(DWORD controlCode);
	static DWORD WINAPI control_handler_ex(DWORD controlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
	static void set_service_description();
	static int pad();
	static void disconnect_remote_sessions();
	static bool IsAnyRDPSessionActive();

	static SERVICE_STATUS serviceStatus;
	static SERVICE_STATUS_HANDLE serviceStatusHandle;
	static char service_path[MAX_PATH];	
	static const char* app_name;
	static char cmdtext[256];
	static char app_path[MAX_PATH];
	static char app_path_UI[MAX_PATH];
	static void monitorSessions();
	static int kickrdp;
	static bool IsShutdown;
	static PROCESS_INFORMATION  ProcessInfo;
	static HANDLE hEndSessionEvent;
	static HANDLE hEvent;
	static int clear_console;

	static void LogError(const char* message);
	static void CleanupHandles(std::initializer_list<HANDLE> handles);
	static void HandlePreconnect(int* sessionData, DWORD& OlddwSessionId);
	static void HandleCADRequest();
	static void LaunchCADProcess();
	static bool MonitorSessions(BOOL RDPMODE, DWORD& dwSessionId, DWORD& OlddwSessionId);
	static void TerminateProcessGracefully(HANDLE hProcess);
	static DWORD GetActiveSessionId();

	static int createWinvncExeCall(bool preconnect, bool rdpselect);
	static BOOL LaunchProcessWin(DWORD dwSessionId, bool preconnect, bool rdpselect);
	static BOOL CreateRemoteSessionProcess(
			IN DWORD        dwSessionId, IN BOOL         bUseDefaultToken,IN HANDLE       hToken,IN LPCWSTR      lpApplicationName,
			IN LPSTR       A_lpCommandLine,IN LPSECURITY_ATTRIBUTES lpProcessAttributes,IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
			IN BOOL bInheritHandles,IN DWORD dwCreationFlags,IN LPVOID lpEnvironment,IN LPCWSTR lpCurrentDirectory,IN LPSTARTUPINFO A_lpStartupInfo, OUT LPPROCESS_INFORMATION lpProcessInformation);
	static DWORD MarshallString(LPCWSTR    pszText, LPVOID, DWORD  dwMaxSize, LPBYTE* ppNextBuf, DWORD* pdwUsedBytes);
	static BOOL Char2Wchar(WCHAR* pDest, char* pSrc, int nDestStrLen);

	static BOOL get_winlogon_handle(OUT LPHANDLE  lphUserToken, DWORD mysessionID);
	static BOOL GetSessionUserTokenWin(OUT LPHANDLE  lphUserToken, DWORD mysessionID);
	static DWORD GetwinlogonPid();
	static DWORD Find_winlogon(DWORD SessionId);
	static BOOL SetTBCPrivileges(VOID);
	static void wait_for_existing_process();
	static bool IsSessionStillActive(int ID);


public:
	UltraVNCService();
	static int start_service(char* cmd);
	static int install_service(void);
	static int uninstall_service(void);

	static BOOL DeleteServiceSafeBootKey();
	static void Restore_safemode();
	static void Restore_after_reboot();

	static char service_name[256];
	static char description[256];
	static char display_name[256];
	static char ultraVNC_Server_UI[256];
	static bool IsServiceInstalled();
	static bool IsServiceRunning();
};
