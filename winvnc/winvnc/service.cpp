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

/*
// vncService

// Implementation of service-oriented functionality of UltraVNC Server
#include <winsock2.h>
#include <windows.h>
#include <userenv.h>
#include <wtsapi32.h>
#include <stdio.h>
#include <tlhelp32.h>
#include "inifile.h"
#include "../common/win32_helpers.h"
#include <tlhelp32.h>

HANDLE hEvent=NULL;
HANDLE hEventcad=NULL;
HANDLE hEventPreConnect = NULL;
HANDLE hMapFile=NULL;
LPVOID data = NULL;
HANDLE hEndSessionEvent = NULL;
static char app_path[MAX_PATH];
typedef DWORD (*WTSGETACTIVECONSOLESESSIONID)();
typedef bool (*WTSQUERYUSERTOKEN)(ULONG,PHANDLE);
helper::DynamicFn<WTSGETACTIVECONSOLESESSIONID> lpfnWTSGetActiveConsoleSessionId("kernel32", "WTSGetActiveConsoleSessionId");
helper::DynamicFn<WTSQUERYUSERTOKEN> lpfnWTSQueryUserToken("Wtsapi32.dll", "WTSQueryUserToken");
PROCESS_INFORMATION  ProcessInfo;
extern char cmdtext[256];
int clear_console=0;

bool Shutdown = false;

static int createWinvncExeCall(bool preconnect, bool rdpselect)
{
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	W2K=0;
	if (OSversion.dwPlatformId == VER_PLATFORM_WIN32_NT && OSversion.dwMajorVersion == 5 && OSversion.dwMinorVersion == 0)
		W2K=1;							    								  
	char exe_file_name[MAX_PATH];
	char cmdline[MAX_PATH];
    GetModuleFileName(0, exe_file_name, MAX_PATH);
    strcpy_s(app_path, exe_file_name);
	if (preconnect)
		strcat_s(app_path, " -preconnect");
	if (rdpselect)
			strcat_s(app_path, " -service_rdp_run");
		else
			strcat_s(app_path, " -service_run");
	IniFile myIniFile;
	kickrdp=myIniFile.ReadInt("admin", "kickrdp", kickrdp);
	clear_console=myIniFile.ReadInt("admin", "clearconsole", clear_console);
	myIniFile.ReadString("admin", "service_commandline",cmdline,256);
	if (strlen(cmdline)!=0) {
		strcpy_s(app_path, exe_file_name);
		if (preconnect)
			strcat_s(app_path, " -preconnect");
		strcat_s(app_path, " ");
		strcat_s(app_path,cmdline);
		if (rdpselect)
			strcat_s(app_path, " -service_rdp_run");
		else
			strcat_s(app_path, " -service_run");
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////////

BOOL SetTBCPrivileges(VOID) {
  DWORD dwPID;
  HANDLE hProcess;
  HANDLE hToken;
  LUID Luid;
  TOKEN_PRIVILEGES tpDebug;
  dwPID = GetCurrentProcessId();
  if ((hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)) == NULL) 
	  return FALSE;
  if (OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken) == 0) {
	  CloseHandle(hProcess);
	  return FALSE;
  }
  if ((LookupPrivilegeValue(NULL, SE_TCB_NAME, &Luid)) == 0) {
	  CloseHandle(hToken);
	  CloseHandle(hProcess);
	  return FALSE;
  }
  tpDebug.PrivilegeCount = 1;
  tpDebug.Privileges[0].Luid = Luid;
  tpDebug.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  if ((AdjustTokenPrivileges(hToken, FALSE, &tpDebug, sizeof(tpDebug), NULL, NULL)) == 0) {
	  CloseHandle(hToken);
	  CloseHandle(hProcess);
	  return FALSE;
  }
  if (GetLastError() != ERROR_SUCCESS) {
	  CloseHandle(hToken);
	  CloseHandle(hProcess);
	  return FALSE;
  }
  CloseHandle(hToken);
  CloseHandle(hProcess);
  return TRUE;
}
//////////////////////////////////////////////////////////////////////////////

DWORD GetwinlogonPid()
{
	DWORD dwExplorerLogonPid=0;
	PROCESSENTRY32 procEntry;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
        return 0 ;

    procEntry.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hSnap, &procEntry)) {
		CloseHandle(hSnap);
        return 0 ;
    }

    do {
        if (_stricmp(procEntry.szExeFile, "winlogon.exe") == 0)
			dwExplorerLogonPid = procEntry.th32ProcessID;

    } while (!Shutdown && Process32Next(hSnap, &procEntry));
	CloseHandle(hSnap);
	return dwExplorerLogonPid;
}
//////////////////////////////////////////////////////////////////////////////
DWORD
Find_winlogon(DWORD SessionId)
{
  PWTS_PROCESS_INFO pProcessInfo = NULL;
  DWORD         ProcessCount = 0;
  DWORD         Id = -1;

  typedef BOOL (WINAPI *pfnWTSEnumerateProcesses)(HANDLE,DWORD,DWORD,PWTS_PROCESS_INFO*,DWORD*);
  typedef VOID (WINAPI *pfnWTSFreeMemory)(PVOID);

  helper::DynamicFn<pfnWTSEnumerateProcesses> pWTSEnumerateProcesses("wtsapi32","WTSEnumerateProcessesA");
  helper::DynamicFn<pfnWTSFreeMemory> pWTSFreeMemory("wtsapi32", "WTSFreeMemory");

  if (pWTSEnumerateProcesses.isValid() && pWTSFreeMemory.isValid()) {
    if ((*pWTSEnumerateProcesses)(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo, &ProcessCount)) {
		// dump each process description
		for (DWORD CurrentProcess = 0; CurrentProcess < ProcessCount; CurrentProcess++) {
			if( _stricmp(pProcessInfo[CurrentProcess].pProcessName, "winlogon.exe") == 0 ) {    
				if (SessionId==pProcessInfo[CurrentProcess].SessionId) {
					Id = pProcessInfo[CurrentProcess].ProcessId;
					break;
				}
			}
		}	
		(*pWTSFreeMemory)(pProcessInfo);
    }
  }
  return Id;
}

//////////////////////////////////////////////////////////////////////////////
BOOL
get_winlogon_handle(OUT LPHANDLE  lphUserToken, DWORD mysessionID)
{
	BOOL   bResult = FALSE;
	HANDLE hProcess;
	HANDLE hAccessToken = NULL;
	HANDLE hTokenThis = NULL;
	DWORD ID_session=0;
	ID_session=mysessionID;
	DWORD Id=0;
	if (W2K==0) 
		Id=Find_winlogon(ID_session);
	else 
		Id=GetwinlogonPid();

    // fall back to old method if Terminal services is disabled
    if (W2K == 0 && Id == -1)
        Id=GetwinlogonPid();

	hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, Id );
	if (hProcess) {
		OpenProcessToken(hProcess, TOKEN_ASSIGN_PRIMARY|TOKEN_ALL_ACCESS, &hTokenThis);
		bResult = DuplicateTokenEx(hTokenThis, TOKEN_ASSIGN_PRIMARY|TOKEN_ALL_ACCESS,NULL, SecurityImpersonation, TokenPrimary, lphUserToken);
		SetTokenInformation(*lphUserToken, TokenSessionId, &ID_session, sizeof(DWORD));
		CloseHandle(hTokenThis);
		CloseHandle(hProcess);
	}
	return bResult;
}
//////////////////////////////////////////////////////////////////////////////

BOOL
GetSessionUserTokenWin(OUT LPHANDLE  lphUserToken,DWORD mysessionID)
{
  BOOL   bResult = FALSE;  
  if (lphUserToken != NULL) {   
		  bResult = get_winlogon_handle(lphUserToken,mysessionID);
  }
  return bResult;
}
//////////////////////////////////////////////////////////////////////////////
// START the app as system 

//////////////////////////////////////////////////////////////////////////////

void wait_for_existing_process()
{
    while (!Shutdown && (hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Global\\SessionEventUltra")) != NULL) {
    	SetEvent(hEvent); // signal Tray icon to shut down 
        CloseHandle(hEvent);
        Sleep(1000);
    }
}
//////////////////////////////////////////////////////////////////////////////
extern SERVICE_STATUS serviceStatus;

bool IsSessionStillActive(int ID)
{
	typedef BOOL (WINAPI *pfnWTSEnumerateSessions)(HANDLE, DWORD, DWORD, PWTS_SESSION_INFO *, DWORD *);;
	typedef VOID (WINAPI *pfnWTSFreeMemory)(PVOID);

	helper::DynamicFn<pfnWTSEnumerateSessions> pWTSEnumerateSessions("wtsapi32","WTSEnumerateSessionsA");
	helper::DynamicFn<pfnWTSFreeMemory> pWTSFreeMemory("wtsapi32", "WTSFreeMemory");
	if (pWTSEnumerateSessions.isValid() && pWTSFreeMemory.isValid()) {
		WTS_SESSION_INFO *pSessions = 0;
		DWORD   nSessions(0);
		DWORD   rdpSessionExists = false;
		if ((*pWTSEnumerateSessions)(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessions, &nSessions)) {
			for (DWORD i(0); i < nSessions && !rdpSessionExists; ++i) {
				//exclude console session
				if ((_stricmp(pSessions[i].pWinStationName, "Console") == 0) && (pSessions[i].SessionId == ID))
					rdpSessionExists = true;
				else if ( (pSessions[i].SessionId==ID) && (pSessions[i].State == WTSActive || pSessions[i].State == WTSShadow || pSessions[i].State == WTSConnectQuery ))
					rdpSessionExists = true;				
			}
			(*pWTSFreeMemory)(pSessions);
		}
		return rdpSessionExists ? true : false;
	}
	return false;
}*/

