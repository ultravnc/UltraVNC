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
#ifndef SC_20
#include "stdhdrs.h"
#include <windows.h>
#include <wtsapi32.h>
#include "common/win32_helpers.h"
#include "common/inifile.h"
#include "UltraVNCService.h"
#include <userenv.h>

char UltraVNCService::service_name[256] = "ultravncserver";

UltraVNCService::UltraVNCService()
{
	
}
////////////////////////////////////////////////////////////////////////////////

// List of other required services ("dependency 1\0dependency 2\0\0")
// *** These need filling in properly
#define VNCDEPENDENCIES    "Tcpip\0\0"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
BOOL UltraVNCService::CreateServiceSafeBootKey()
{
	HKEY hKey;
	DWORD dwDisp = 0;
	LONG lSuccess;
	char szKey[1024];
	_snprintf_s(szKey, 1024, "SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\%s\\%s", "Network", service_name);
	lSuccess = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKey, 0L, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);
	if (lSuccess == ERROR_SUCCESS)
	{
    RegSetValueEx(hKey, NULL, 0, REG_SZ, (unsigned char*)"Service", 8);
		RegCloseKey(hKey);
		return TRUE;
	}
	else
		return FALSE;
}
////////////////////////////////////////////////////////////////////////////////
void UltraVNCService::Set_Safemode()
{
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if(OSversion.dwMajorVersion<6)
			{
					char drivepath[150];
					char systemdrive[150];
					char stringvalue[512];
					GetEnvironmentVariable("SYSTEMDRIVE", systemdrive, 150);
					strcat_s(systemdrive,"/boot.ini");
					GetPrivateProfileString("boot loader","default","",drivepath,150,systemdrive);
					if (strlen(drivepath)==0) return;
					GetPrivateProfileString("operating systems",drivepath,"",stringvalue,512,systemdrive);
					if (strlen(stringvalue)==0) return;
					strcat_s(stringvalue," /safeboot:network");
					SetFileAttributes(systemdrive,FILE_ATTRIBUTE_NORMAL);
					WritePrivateProfileString("operating systems",drivepath,stringvalue,systemdrive);
					DWORD err=GetLastError();


			}
			else
			{

#ifdef _X64
			char systemroot[150];
			GetEnvironmentVariable("SystemRoot", systemroot, 150);
			char exe_file_name[MAX_PATH];
			char parameters[MAX_PATH];
			strcpy_s(exe_file_name,systemroot);
			strcat_s(exe_file_name,"\\system32\\");
			strcat_s(exe_file_name,"bcdedit.exe");
			strcpy_s(parameters,"/set safeboot network");
			SHELLEXECUTEINFO shExecInfo;
			shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
			shExecInfo.fMask = NULL;
			shExecInfo.hwnd = GetForegroundWindow();
			shExecInfo.lpVerb = "runas";
			shExecInfo.lpFile = exe_file_name;
			shExecInfo.lpParameters = parameters;
			shExecInfo.lpDirectory = NULL;
			shExecInfo.nShow = SW_HIDE;
			shExecInfo.hInstApp = NULL;
			ShellExecuteEx(&shExecInfo);
#else

			typedef BOOL (WINAPI *LPFN_Wow64DisableWow64FsRedirection)(PVOID* OldValue);
			typedef BOOL (WINAPI *LPFN_Wow64RevertWow64FsRedirection)(PVOID OldValue);
			PVOID OldValue;  
			LPFN_Wow64DisableWow64FsRedirection pfnWow64DisableWowFsRedirection = (LPFN_Wow64DisableWow64FsRedirection)GetProcAddress(GetModuleHandle("kernel32"),"Wow64DisableWow64FsRedirection");
			LPFN_Wow64RevertWow64FsRedirection pfnWow64RevertWow64FsRedirection = (LPFN_Wow64RevertWow64FsRedirection)GetProcAddress(GetModuleHandle("kernel32"),"Wow64RevertWow64FsRedirection");
			if (pfnWow64DisableWowFsRedirection && pfnWow64RevertWow64FsRedirection) 
			{
				if(TRUE == pfnWow64DisableWowFsRedirection(&OldValue))
					{
						char systemroot[150];
						GetEnvironmentVariable("SystemRoot", systemroot, 150);
						char exe_file_name[MAX_PATH];
						char parameters[MAX_PATH];
						strcpy_s(exe_file_name,systemroot);
						strcat_s(exe_file_name,"\\system32\\");
						strcat_s(exe_file_name,"bcdedit.exe");
						strcpy_s(parameters,"/set safeboot network");
						SHELLEXECUTEINFO shExecInfo;
						shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
						shExecInfo.fMask = NULL;
						shExecInfo.hwnd = GetForegroundWindow();
						shExecInfo.lpVerb = "runas";
						shExecInfo.lpFile = exe_file_name;
						shExecInfo.lpParameters = parameters;
						shExecInfo.lpDirectory = NULL;
						shExecInfo.nShow = SW_HIDE;
						shExecInfo.hInstApp = NULL;
						ShellExecuteEx(&shExecInfo);
						pfnWow64RevertWow64FsRedirection(OldValue);
					}
				else
				{
					char systemroot[150];
					GetEnvironmentVariable("SystemRoot", systemroot, 150);
					char exe_file_name[MAX_PATH];
					char parameters[MAX_PATH];
					strcpy_s(exe_file_name,systemroot);
					strcat_s(exe_file_name,"\\system32\\");
					strcat_s(exe_file_name,"bcdedit.exe");
					strcpy_s(parameters,"/set safeboot network");
					SHELLEXECUTEINFO shExecInfo;
					shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
					shExecInfo.fMask = NULL;
					shExecInfo.hwnd = GetForegroundWindow();
					shExecInfo.lpVerb = "runas";
					shExecInfo.lpFile = exe_file_name;
					shExecInfo.lpParameters = parameters;
					shExecInfo.lpDirectory = NULL;
					shExecInfo.nShow = SW_HIDE;
					shExecInfo.hInstApp = NULL;
					ShellExecuteEx(&shExecInfo);
				}
			}
			else
			{
				char systemroot[150];
				GetEnvironmentVariable("SystemRoot", systemroot, 150);
				char exe_file_name[MAX_PATH];
				char parameters[MAX_PATH];
				strcpy_s(exe_file_name,systemroot);
				strcat_s(exe_file_name,"\\system32\\");
				strcat_s(exe_file_name,"bcdedit.exe");
				strcpy_s(parameters,"/set safeboot network");
				SHELLEXECUTEINFO shExecInfo;
				shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
				shExecInfo.fMask = NULL;
				shExecInfo.hwnd = GetForegroundWindow();
				shExecInfo.lpVerb = "runas";
				shExecInfo.lpFile = exe_file_name;
				shExecInfo.lpParameters = parameters;
				shExecInfo.lpDirectory = NULL;
				shExecInfo.nShow = SW_HIDE;
				shExecInfo.hInstApp = NULL;
				ShellExecuteEx(&shExecInfo);
			}
#endif

			}
}

BOOL UltraVNCService::reboot()
{
	HANDLE hToken; 
    TOKEN_PRIVILEGES tkp; 
    if (OpenProcessToken(    GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
                & hToken)) 
		{
			LookupPrivilegeValue(    NULL,  SE_SHUTDOWN_NAME,  & tkp.Privileges[0].Luid);          
			tkp.PrivilegeCount = 1; 
			tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
			if(AdjustTokenPrivileges(    hToken,  FALSE,  & tkp,  0,  (PTOKEN_PRIVILEGES)NULL,  0))
				{
					ExitWindowsEx(EWX_REBOOT, 0);
				}
		}
	return TRUE;
}

BOOL UltraVNCService::Force_reboot()
{
	HANDLE hToken; 
    TOKEN_PRIVILEGES tkp; 
    if (OpenProcessToken(    GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
                & hToken)) 
		{
			LookupPrivilegeValue(    NULL,  SE_SHUTDOWN_NAME,  & tkp.Privileges[0].Luid);          
			tkp.PrivilegeCount = 1; 
			tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
			if(AdjustTokenPrivileges(    hToken,  FALSE,  & tkp,  0,  (PTOKEN_PRIVILEGES)NULL,  0))
				{
					OSVERSIONINFO OSversion;	
					OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
					GetVersionEx(&OSversion);
					if(OSversion.dwMajorVersion<6)
					{
					ExitWindowsEx(EWX_REBOOT|EWX_FORCEIFHUNG, 0);
					}
					else
					{
					ExitWindowsEx(EWX_REBOOT|EWX_FORCE, 0);
					}
				}
		}
	return TRUE;
}

void UltraVNCService::Reboot_with_force_reboot()
{
	Force_reboot();

}

void UltraVNCService::Reboot_with_force_reboot_elevated()
{
	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);
	SHELLEXECUTEINFO shExecInfo;
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = "-rebootforce";
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_HIDE;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
}

void UltraVNCService::Reboot_in_safemode()
{
	if (CreateServiceSafeBootKey()) 
		{
			Set_Safemode();
			reboot();
		}

}

void UltraVNCService::Reboot_in_safemode_elevated()
{
	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);
	SHELLEXECUTEINFO shExecInfo;
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = "-rebootsafemode";
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_HIDE;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
}

////////////////// ALL /////////////////////////////
////////////////////////////////////////////////////

void UltraVNCService::Restore_safemode()
{
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if(OSversion.dwMajorVersion<6)
		{
			char drivepath[150];
			char systemdrive[150];
			char stringvalue[512];
			GetEnvironmentVariable("SYSTEMDRIVE", systemdrive, 150);
			strcat_s(systemdrive,"/boot.ini");
			GetPrivateProfileString("boot loader","default","",drivepath,150,systemdrive);
			if (strlen(drivepath)==0) return;
			GetPrivateProfileString("operating systems",drivepath,"",stringvalue,512,systemdrive);
			if (strlen(stringvalue)==0) return;
			char* p = strrchr(stringvalue, '/');
			if (p == NULL) return;
				*p = '\0';
			WritePrivateProfileString("operating systems",drivepath,stringvalue,systemdrive);		
			SetFileAttributes(systemdrive,FILE_ATTRIBUTE_READONLY);
		}
	else
		{
#ifdef _X64
			char systemroot[150];
			GetEnvironmentVariable("SystemRoot", systemroot, 150);
			char exe_file_name[MAX_PATH];
			char parameters[MAX_PATH];
			strcpy_s(exe_file_name,systemroot);
			strcat_s(exe_file_name,"\\system32\\");
			strcat_s(exe_file_name,"bcdedit.exe");
			strcpy_s(parameters,"/deletevalue safeboot");
			SHELLEXECUTEINFO shExecInfo;
			shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
			shExecInfo.fMask = NULL;
			shExecInfo.hwnd = GetForegroundWindow();
			shExecInfo.lpVerb = "runas";
			shExecInfo.lpFile = exe_file_name;
			shExecInfo.lpParameters = parameters;
			shExecInfo.lpDirectory = NULL;
			shExecInfo.nShow = SW_HIDE;
			shExecInfo.hInstApp = NULL;
			ShellExecuteEx(&shExecInfo);
#else
			typedef BOOL (WINAPI *LPFN_Wow64DisableWow64FsRedirection)(PVOID* OldValue);
			typedef BOOL (WINAPI *LPFN_Wow64RevertWow64FsRedirection)(PVOID OldValue);
			PVOID OldValue;  
			LPFN_Wow64DisableWow64FsRedirection pfnWow64DisableWowFsRedirection = (LPFN_Wow64DisableWow64FsRedirection)GetProcAddress(GetModuleHandle("kernel32"),"Wow64DisableWow64FsRedirection");
			LPFN_Wow64RevertWow64FsRedirection pfnWow64RevertWow64FsRedirection = (LPFN_Wow64RevertWow64FsRedirection)GetProcAddress(GetModuleHandle("kernel32"),"Wow64RevertWow64FsRedirection");
			if (pfnWow64DisableWowFsRedirection && pfnWow64RevertWow64FsRedirection)  ///win32 on x64 system
			{
				if(TRUE == pfnWow64DisableWowFsRedirection(&OldValue))
					{
						char systemroot[150];
						GetEnvironmentVariable("SystemRoot", systemroot, 150);
						char exe_file_name[MAX_PATH];
						char parameters[MAX_PATH];
						strcpy_s(exe_file_name,systemroot);
						strcat_s(exe_file_name,"\\system32\\");
						strcat_s(exe_file_name,"bcdedit.exe");
						strcpy_s(parameters,"/deletevalue safeboot");
						SHELLEXECUTEINFO shExecInfo;
						shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
						shExecInfo.fMask = NULL;
						shExecInfo.hwnd = GetForegroundWindow();
						shExecInfo.lpVerb = "runas";
						shExecInfo.lpFile = exe_file_name;
						shExecInfo.lpParameters = parameters;
						shExecInfo.lpDirectory = NULL;
						shExecInfo.nShow = SW_HIDE;
						shExecInfo.hInstApp = NULL;
						ShellExecuteEx(&shExecInfo);
						pfnWow64RevertWow64FsRedirection(OldValue);
					}
				else
				{
					char systemroot[150];
					GetEnvironmentVariable("SystemRoot", systemroot, 150);
					char exe_file_name[MAX_PATH];
					char parameters[MAX_PATH];
					strcpy_s(exe_file_name,systemroot);
					strcat_s(exe_file_name,"\\system32\\");
					strcat_s(exe_file_name,"bcdedit.exe");
					strcpy_s(parameters,"/deletevalue safeboot");
					SHELLEXECUTEINFO shExecInfo;
					shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
					shExecInfo.fMask = NULL;
					shExecInfo.hwnd = GetForegroundWindow();
					shExecInfo.lpVerb = "runas";
					shExecInfo.lpFile = exe_file_name;
					shExecInfo.lpParameters = parameters;
					shExecInfo.lpDirectory = NULL;
					shExecInfo.nShow = SW_HIDE;
					shExecInfo.hInstApp = NULL;
					ShellExecuteEx(&shExecInfo);
				}
			}
			else  //win32 on W32
			{
				char systemroot[150];
				GetEnvironmentVariable("SystemRoot", systemroot, 150);
				char exe_file_name[MAX_PATH];
				char parameters[MAX_PATH];
				strcpy_s(exe_file_name,systemroot);
				strcat_s(exe_file_name,"\\system32\\");
				strcat_s(exe_file_name,"bcdedit.exe");
				strcpy_s(parameters,"/deletevalue safeboot");
				SHELLEXECUTEINFO shExecInfo;
				shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
				shExecInfo.fMask = NULL;
				shExecInfo.hwnd = GetForegroundWindow();
				shExecInfo.lpVerb = "runas";
				shExecInfo.lpFile = exe_file_name;
				shExecInfo.lpParameters = parameters;
				shExecInfo.lpDirectory = NULL;
				shExecInfo.nShow = SW_HIDE;
				shExecInfo.hInstApp = NULL;
				ShellExecuteEx(&shExecInfo);
			}
#endif
		}
}

#endif
