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
#include <shlobj.h>
#include <direct.h>
#include <fstream>



SERVICE_STATUS UltraVNCService::serviceStatus{};
SERVICE_STATUS_HANDLE UltraVNCService::serviceStatusHandle = NULL;
char UltraVNCService::service_path[MAX_PATH]{};
char UltraVNCService::service_name[256] = "uvnc_service";
char UltraVNCService::app_path[MAX_PATH]{};
int UltraVNCService::kickrdp = 0;
PROCESS_INFORMATION  UltraVNCService::ProcessInfo{};
bool UltraVNCService::IsShutdown = false;
HANDLE UltraVNCService::hEndSessionEvent = NULL;
HANDLE UltraVNCService::hEvent = NULL;
int UltraVNCService::clear_console = 0;
char* UltraVNCService::app_name = "UltraVNC";
char  UltraVNCService::cmdtext[256]{};
char UltraVNCService::configfilename[MAX_PATH] = "";
char UltraVNCService::inifile[MAX_PATH] = "";
IniFile UltraVNCService::iniFileService;

void GetServiceExecutablePath(char* path, size_t size) {
	// Use GetModuleFileName to retrieve the executable path
	DWORD result = GetModuleFileNameA(nullptr, path, static_cast<DWORD>(size));
	if (result == 0) {
		path[0] = '\0'; // Ensure the path is empty on failure
	}
}

UltraVNCService::UltraVNCService()
{
	
}

////////////////////////////////////////////////////////////////////////////////
void WINAPI UltraVNCService::service_main(DWORD argc, LPTSTR* argv) {
    /* initialise service status */
    serviceStatus.dwServiceType=SERVICE_WIN32;
    serviceStatus.dwCurrentState=SERVICE_STOPPED;
    serviceStatus.dwControlsAccepted=0;
    serviceStatus.dwWin32ExitCode=NO_ERROR;
    serviceStatus.dwServiceSpecificExitCode=NO_ERROR;
    serviceStatus.dwCheckPoint=0;
    serviceStatus.dwWaitHint=0;
	if (strcmp(argv[0], service_name) == NULL) {
		strcpy_s(configfilename, "ultravnc.ini");
	}
	else
	{
		strcpy_s(configfilename, argv[0]);
		strcat_s(configfilename, ".ini");
	}

	char programdataPath[MAX_PATH]{};
	char currentFolder[MAX_PATH]{};
	HRESULT result = SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, programdataPath);
	strcpy_s(inifile, "");
	strcat_s(inifile, programdataPath);
	strcat_s(inifile, "\\UltraVNC");
	strcat_s(inifile, "\\");
	strcat_s(inifile, configfilename);
	std::ifstream file(inifile);
	if (!file.good()) {
		GetServiceExecutablePath(currentFolder, MAX_PATH);
		strcpy_s(inifile, "");
		strcat_s(inifile, currentFolder);
		strcat_s(inifile, "\\");
		strcat_s(inifile, "ultravnc.ini");
	}

	iniFileService.setIniFile(inifile);

    typedef SERVICE_STATUS_HANDLE (WINAPI * pfnRegisterServiceCtrlHandlerEx)(LPCTSTR, LPHANDLER_FUNCTION_EX, LPVOID);
    helper::DynamicFn<pfnRegisterServiceCtrlHandlerEx> pRegisterServiceCtrlHandlerEx("advapi32.dll","RegisterServiceCtrlHandlerExA");

    if (pRegisterServiceCtrlHandlerEx.isValid())
      serviceStatusHandle = (*pRegisterServiceCtrlHandlerEx)(service_name, control_handler_ex, 0);
    else 
      serviceStatusHandle = RegisterServiceCtrlHandler(service_name, control_handler);

    if(serviceStatusHandle) {
        /* service is starting */
        serviceStatus.dwCurrentState=SERVICE_START_PENDING;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);

        /* running */
        serviceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
        serviceStatus.dwControlsAccepted |= SERVICE_ACCEPT_SESSIONCHANGE | SERVICE_CONTROL_PRESHUTDOWN;

        serviceStatus.dwCurrentState=SERVICE_RUNNING;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);

		Restore_after_reboot();
		
		while (serviceStatus.dwCurrentState == SERVICE_RUNNING) {
			monitorSessions();
			Sleep(3000);
		}


        /* service was stopped */
        serviceStatus.dwCurrentState=SERVICE_STOP_PENDING;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);

        /* service is now stopped */
        serviceStatus.dwControlsAccepted&=
            ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
        serviceStatus.dwCurrentState=SERVICE_STOPPED;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);
    }
}
////////////////////////////////////////////////////////////////////////////////
void WINAPI UltraVNCService::control_handler(DWORD controlCode)
{
  control_handler_ex(controlCode, 0, 0, 0);
}
DWORD WINAPI UltraVNCService::control_handler_ex(DWORD controlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
    switch (controlCode) {
    case SERVICE_CONTROL_INTERROGATE:
        break;

	case SERVICE_CONTROL_PRESHUTDOWN:
    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:
        serviceStatus.dwCurrentState=SERVICE_STOP_PENDING;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);
        SetEvent(hEndSessionEvent);
		SetEvent(hEvent);
        return NO_ERROR;

    case SERVICE_CONTROL_PAUSE:
        break;

    case SERVICE_CONTROL_CONTINUE:
        break;

    case SERVICE_CONTROL_SESSIONCHANGE:
        {
            if (dwEventType == WTS_REMOTE_DISCONNECT)
            {
                // disconnect rdp, and reconnect to the console
                if ( clear_console) disconnect_remote_sessions();
            }
        }
        break;

    default:
        if(controlCode >= 128 && controlCode <= 255)
            break; 
        else
            break;
    }
    SetServiceStatus(serviceStatusHandle, &serviceStatus);
    return NO_ERROR;
}
////////////////////////////////////////////////////////////////////////////////
int UltraVNCService::start_service(char *cmd) {
	strcpy_s(cmdtext,256,cmd);
    SERVICE_TABLE_ENTRY serviceTable[]={
	 {service_name, service_main},
        {0, 0}
    };

    if(!StartServiceCtrlDispatcher(serviceTable)) {
        return 1;
    }
    return 0; /* NT service started */
}
////////////////////////////////////////////////////////////////////////////////
void UltraVNCService::set_service_description()
{
    // Add service description 
	DWORD	dw;
	HKEY hKey;
	char tempName[256];
    char desc[] = "Provides secure remote desktop sharing";
	_snprintf_s(tempName,  sizeof tempName, "SYSTEM\\CurrentControlSet\\Services\\%s", service_name);
	RegCreateKeyEx(HKEY_LOCAL_MACHINE,
						tempName,
						0,
						REG_NONE,
						REG_OPTION_NON_VOLATILE,
						KEY_READ|KEY_WRITE,
						NULL,
						&hKey,
						&dw);
	RegSetValueEx(hKey,
					"Description",
					0,
					REG_SZ,
					(const BYTE *)desc,
					strlen(desc)+1);


	RegCloseKey(hKey);
}


// List of other required services ("dependency 1\0dependency 2\0\0")
// *** These need filling in properly
#define VNCDEPENDENCIES    "Tcpip\0\0"



int UltraVNCService::install_service(void) {
    SC_HANDLE scm, service;
	pad();

    scm=OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
    if(!scm) {
        MessageBoxSecure(NULL, "Failed to open service control manager",
            app_name, MB_ICONERROR);
        return 1;
    }
    //"Provides secure remote desktop sharing"
    service=CreateService(scm,service_name, service_name, SERVICE_ALL_ACCESS,
                          SERVICE_WIN32_OWN_PROCESS,
                          SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, service_path,
        NULL, NULL, VNCDEPENDENCIES, NULL, NULL);
    if(!service) {
		DWORD myerror=GetLastError();
		if (myerror==ERROR_ACCESS_DENIED)
		{
			MessageBoxSecure(NULL, "Failed: Permission denied",
            app_name, MB_ICONERROR);
			CloseServiceHandle(scm);
			return 1;
		}
		if (myerror==ERROR_SERVICE_EXISTS)
		{
			CloseServiceHandle(scm);
			return 1;
		}

        MessageBoxSecure(NULL, "Failed to create a new service",
            app_name, MB_ICONERROR);
        CloseServiceHandle(scm);
        return 1;
    }
    else
        set_service_description();
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
int UltraVNCService::uninstall_service(void) {
    SC_HANDLE scm, service;
    SERVICE_STATUS serviceStatus;

    scm=OpenSCManager(0, 0, SC_MANAGER_CONNECT);
    if(!scm) {
        MessageBoxSecure(NULL, "Failed to open service control manager",
            app_name, MB_ICONERROR);
        return 1;
    }

	service=OpenService(scm, service_name,
        SERVICE_QUERY_STATUS | DELETE);
    if(!service) {
		DWORD myerror=GetLastError();
		if (myerror==ERROR_ACCESS_DENIED)
		{
			MessageBoxSecure(NULL, "Failed: Permission denied",
            app_name, MB_ICONERROR);
			CloseServiceHandle(scm);
			return 1;
		}
		if (myerror==ERROR_SERVICE_DOES_NOT_EXIST)
		{
#if 0
			MessageBoxSecure(NULL, "Failed: Service is not installed",
            app_name, MB_ICONERROR);
#endif
			CloseServiceHandle(scm);
			return 1;
		}

        MessageBoxSecure(NULL, "Failed to open the service",
            app_name, MB_ICONERROR);
        CloseServiceHandle(scm);
        return 1;
    }
    if(!QueryServiceStatus(service, &serviceStatus)) {
        MessageBoxSecure(NULL, "Failed to query service status",
            app_name, MB_ICONERROR);
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return 1;
    }
    if(serviceStatus.dwCurrentState!=SERVICE_STOPPED) {
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
		Sleep(2500);uninstall_service();
        return 1;
    }
    if(!DeleteService(service)) {
        MessageBoxSecure(NULL, "Failed to delete the service",
            app_name, MB_ICONERROR);
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return 1;
    }
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
int UltraVNCService::pad()
{
	char exe_file_name[MAX_PATH], dir[MAX_PATH], *ptr;
    GetModuleFileName(0, exe_file_name, MAX_PATH);

    /* set current directory */
    strcpy_s(dir, exe_file_name);
    ptr=strrchr(dir, '\\'); /* last backslash */
    if(ptr)
        ptr[1]='\0'; /* truncate program name */
    if(!SetCurrentDirectory(dir)) {
        return 1;
    }

    strcpy_s(service_path, "\"");
    strcat_s(service_path, exe_file_name);
	strcat_s(service_path, "\"");
	strcat_s(service_path, " -service");
	return 0;
}
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

BOOL UltraVNCService::DeleteServiceSafeBootKey()
{
	LONG lSuccess;
	char szKey[1024];
	_snprintf_s(szKey, 1024, "SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\%s\\%s", "Network", service_name);
	lSuccess = RegDeleteKey(HKEY_LOCAL_MACHINE, szKey);
	return lSuccess == ERROR_SUCCESS;

}

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

void UltraVNCService::Restore_after_reboot()
{
	//If we are running !normal mode
	//disable boot.ini /safemode:network
	//disable safeboot service
	if (GetSystemMetrics(SM_CLEANBOOT) != 0)
	{
		Restore_safemode();
		DeleteServiceSafeBootKey();
	}
}

void UltraVNCService::disconnect_remote_sessions()
{
	typedef BOOLEAN(WINAPI* pWinStationConnect) (HANDLE, ULONG, ULONG, PCWSTR, ULONG);
	HMODULE  hlibwinsta = LoadLibrary("winsta.dll");
	pWinStationConnect WinStationConnectF = NULL;

	// don't kick rdp off if there's still an active session
	if (IsAnyRDPSessionActive())
		return;
	if (hlibwinsta)
		WinStationConnectF = (pWinStationConnect)GetProcAddress(hlibwinsta, "WinStationConnectW");
	if (WinStationConnectF != NULL) {
		DWORD ID = WTSGetActiveConsoleSessionId();
		WinStationConnectF(0, 0, ID, L"", 0);
		// sleep to allow the system to finish the connect/disconnect process. If we don't
		// then the workstation won't get locked every time.
		Sleep(3000);
		if (!LockWorkStation()) {
			char msg[1024];
			sprintf_s(msg, "LockWorkstation failed with error 0x%0lX", GetLastError());
			::OutputDebugString(msg);
		}
	}
	Sleep(3000);
	if (hlibwinsta)
		FreeLibrary(hlibwinsta);
}

bool UltraVNCService::IsAnyRDPSessionActive()
{
	WTS_SESSION_INFO* pSessions = 0;
	DWORD   nSessions(0);
	DWORD   rdpSessionExists = false;

	typedef BOOL(WINAPI* pfnWTSEnumerateSessions)(HANDLE, DWORD, DWORD, PWTS_SESSION_INFO*, DWORD*);
	typedef VOID(WINAPI* pfnWTSFreeMemory)(PVOID);

	helper::DynamicFn<pfnWTSEnumerateSessions> pWTSEnumerateSessions("wtsapi32", "WTSEnumerateSessionsA");
	helper::DynamicFn<pfnWTSFreeMemory> pWTSFreeMemory("wtsapi32", "WTSFreeMemory");

	if (pWTSEnumerateSessions.isValid() && pWTSFreeMemory.isValid())
		if ((*pWTSEnumerateSessions)(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessions, &nSessions)) {
			for (DWORD i(0); i < nSessions && !rdpSessionExists; ++i) {
				if ((_stricmp(pSessions[i].pWinStationName, "Console") != 0) &&
					(pSessions[i].State == WTSActive ||
						pSessions[i].State == WTSShadow ||
						pSessions[i].State == WTSConnectQuery
						))
					rdpSessionExists = true;
			}
			(*pWTSFreeMemory)(pSessions);
		}
	return rdpSessionExists ? true : false;
}


int UltraVNCService::createWinvncExeCall(bool preconnect, bool rdpselect)
{
	OSVERSIONINFO OSversion;
	OSversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	char exe_file_name[MAX_PATH];
	char cmdline[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);
	strcpy_s(app_path, exe_file_name);
	strcat_s(app_path, " -config");
	strcat_s(app_path, "\"");
	strcat_s(app_path, UltraVNCService::inifile);
	strcat_s(app_path, "\"");
	if (preconnect)
		strcat_s(app_path, " -preconnect");
	if (rdpselect)
		strcat_s(app_path, " -service_rdp_run");
	else
		strcat_s(app_path, " -service_run");

	kickrdp = iniFileService.ReadInt("admin", "kickrdp", kickrdp);
	clear_console = iniFileService.ReadInt("admin", "clearconsole", clear_console);
	iniFileService.ReadString("admin", "service_commandline", cmdline, 256);
	if (strlen(cmdline) != 0) {
		strcpy_s(app_path, exe_file_name);
		if (preconnect)
			strcat_s(app_path, " -preconnect");
		strcat_s(app_path, " ");
		strcat_s(app_path, cmdline);
		if (rdpselect)
			strcat_s(app_path, " -service_rdp_run");
		else
			strcat_s(app_path, " -service_run");
	}
	return 0;
}


void UltraVNCService::monitorSessions() {
	BOOL  RDPMODE = false;
	RDPMODE = iniFileService.ReadInt("admin", "rdpmode", 0);
	createWinvncExeCall(false, false);
	DWORD requestedSessionID = 0;
	DWORD dwSessionId = 0;
	DWORD OlddwSessionId = 99;
	ProcessInfo.hProcess = 0;
	HANDLE testevent3[3];
	HANDLE testevent2[2];
	IsShutdown = false;
	bool preconnect_start = false;
	HANDLE hEvent = NULL;
	HANDLE hEventcad = NULL;
	HANDLE hEventPreConnect = NULL;
	HANDLE hMapFile = NULL;
	LPVOID data = NULL;

	//We use this event to notify the program that the session has changed
	//The program need to end so the service can restart the program in the correct session
	wait_for_existing_process();
	hEvent = CreateEvent(NULL, FALSE, FALSE, "Global\\SessionEventUltra");
	hEventcad = CreateEvent(NULL, FALSE, FALSE, "Global\\SessionEventUltraCad");
	hEventPreConnect = CreateEvent(NULL, FALSE, FALSE, "Global\\SessionEventUltraPreConnect");
	hEndSessionEvent = CreateEvent(NULL, FALSE, FALSE, "Global\\EndSessionEvent");
	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), "Global\\SessionUltraPreConnect");
	if (hMapFile)data = MapViewOfFile(hMapFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	Sleep(3000);
	int* a = (int*)data;
	testevent3[0] = hEndSessionEvent;
	testevent3[1] = hEventcad;
	testevent3[2] = hEventPreConnect;
	testevent2[0] = hEndSessionEvent;
	testevent2[1] = hEventcad;

	//IsAnyRDPSessionActive()
	while (!IsShutdown && serviceStatus.dwCurrentState == SERVICE_RUNNING) {
		DWORD dwEvent;
		if (RDPMODE)
			dwEvent = WaitForMultipleObjects(3, testevent3, FALSE, 1000);
		else
			dwEvent = WaitForMultipleObjects(2, testevent2, FALSE, 1000);

		switch (dwEvent) {

			// We get some preconnect session selection input
		case WAIT_OBJECT_0 + 2:
		{
			//Tell UltraVNC Server to stop
			SetEvent(hEvent);
			requestedSessionID = *a;
			//We always have a process handle, else we could not get the signal from it.
			DWORD dwCode = STILL_ACTIVE;
			while (dwCode == STILL_ACTIVE && ProcessInfo.hProcess != NULL && !IsShutdown) {
				GetExitCodeProcess(ProcessInfo.hProcess, &dwCode);
				if (dwCode != STILL_ACTIVE) {
					WaitForSingleObject(ProcessInfo.hProcess, 15000);
					if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
					if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
				}
				else
					Sleep(1000);
			}

			dwSessionId = 0xFFFFFFFF;
			int sessidcounter = 0;
			while (dwSessionId == 0xFFFFFFFF && !IsShutdown) {
				dwSessionId = WTSGetActiveConsoleSessionId();
				Sleep(1000);
				sessidcounter++;
				if (sessidcounter > 10) break;
			}
			LaunchProcessWin(requestedSessionID, false, true);
			OlddwSessionId = requestedSessionID;
			preconnect_start = true;
		}
		break;

		//stopServiceEvent, exit while loop
		case WAIT_OBJECT_0 + 0:
			IsShutdown = true;
			break;

			//cad request
		case WAIT_OBJECT_0 + 1:
		{
			typedef VOID(WINAPI* SendSas)(BOOL asUser);
			HINSTANCE Inst = LoadLibrary("sas.dll");
			SendSas sendSas = (SendSas)GetProcAddress(Inst, "SendSAS");
			if (sendSas)
				sendSas(FALSE);
			else {
				char WORKDIR[MAX_PATH];
				char mycommand[MAX_PATH];
				if (GetModuleFileName(NULL, WORKDIR, MAX_PATH)) {
					char* p = strrchr(WORKDIR, '\\');
					if (p == NULL) return;
					*p = '\0';
				}
				strcpy_s(mycommand, "");
				strcat_s(mycommand, WORKDIR);//set the directory
				strcat_s(mycommand, "\\");
				strcat_s(mycommand, "cad.exe");
				(void)ShellExecute(GetDesktopWindow(), "open", mycommand, "", 0, SW_SHOWNORMAL);
			}
			if (Inst)
				FreeLibrary(Inst);
		}
		break;

		case WAIT_TIMEOUT:
			if (RDPMODE && IsAnyRDPSessionActive()) {
				//First RUN	
				if (ProcessInfo.hProcess == NULL) {
					if (IsAnyRDPSessionActive()) {
						LaunchProcessWin(0, true, false);
						OlddwSessionId = 0;
						preconnect_start = false;
						goto whileloop;
					}
					else {
						dwSessionId = 0xFFFFFFFF;
						int sessidcounter = 0;
						while (dwSessionId == 0xFFFFFFFF && !IsShutdown) {
							dwSessionId = WTSGetActiveConsoleSessionId();
							Sleep(1000);
							sessidcounter++;
							if (sessidcounter > 10) break;
						}
						LaunchProcessWin(dwSessionId, false, false);
						OlddwSessionId = dwSessionId;
						preconnect_start = false;
						goto whileloop;
					}
				}

				if (preconnect_start == true)
					if (!IsSessionStillActive(OlddwSessionId)) SetEvent(hEvent);

				// Monitor process
				DWORD dwCode = 0;
				bool returnvalue = GetExitCodeProcess(ProcessInfo.hProcess, &dwCode);
				if (!returnvalue) {
					//bad handle, thread already terminated
					if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
					if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
					ProcessInfo.hProcess = NULL;
					ProcessInfo.hThread = NULL;
					RDPMODE = iniFileService.ReadInt("admin", "rdpmode", 0);
					Sleep(1000);
					goto whileloop;
				}

				if (dwCode == STILL_ACTIVE)
					goto whileloop;
				if (ProcessInfo.hProcess)
					WaitForSingleObject(ProcessInfo.hProcess, 15000);
				if (ProcessInfo.hProcess)
					CloseHandle(ProcessInfo.hProcess);
				if (ProcessInfo.hThread)
					CloseHandle(ProcessInfo.hThread);
				ProcessInfo.hProcess = NULL;
				ProcessInfo.hThread = NULL;
				RDPMODE = iniFileService.ReadInt("admin", "rdpmode", 0);
				Sleep(1000);
				goto whileloop;
			}//timeout
			else
			{
					dwSessionId = WTSGetActiveConsoleSessionId();
				if (OlddwSessionId != dwSessionId)
					SetEvent(hEvent);
				if (dwSessionId != 0xFFFFFFFF) {
					DWORD dwCode = 0;
					if (ProcessInfo.hProcess == NULL) {
						//First RUNf
						LaunchProcessWin(dwSessionId, false, false);
						OlddwSessionId = dwSessionId;
					}
					else if (GetExitCodeProcess(ProcessInfo.hProcess, &dwCode)) {
						if (dwCode != STILL_ACTIVE) {
							WaitForSingleObject(ProcessInfo.hProcess, 15000);
							if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
							if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
							ProcessInfo.hProcess = NULL;
							ProcessInfo.hThread = NULL;
							int sessidcounter = 0;
							while (dwSessionId == 0xFFFFFFFF && !IsShutdown) {
								Sleep(1000);
								dwSessionId = WTSGetActiveConsoleSessionId();
								sessidcounter++;
								if (sessidcounter > 10) break;
							}
							RDPMODE = iniFileService.ReadInt("admin", "rdpmode", 0);
							goto whileloop;
						}
					}
					else {
						if (ProcessInfo.hProcess)
							CloseHandle(ProcessInfo.hProcess);
						if (ProcessInfo.hThread)
							CloseHandle(ProcessInfo.hThread);
						ProcessInfo.hProcess = NULL;
						ProcessInfo.hThread = NULL;
						int sessidcounter = 0;

						while (dwSessionId == 0xFFFFFFFF && !IsShutdown) {
							Sleep(1000);
							dwSessionId = WTSGetActiveConsoleSessionId();
							sessidcounter++;
							if (sessidcounter > 10)
								break;
						}

						RDPMODE = iniFileService.ReadInt("admin", "rdpmode", 0);
						goto whileloop;
					}
				}
			}


		}//switch

	whileloop:
		;
	}//while

	if (hEvent)
		SetEvent(hEvent);
	if (ProcessInfo.hProcess) {
		WaitForSingleObject(ProcessInfo.hProcess, 15000);
		if (ProcessInfo.hProcess)
			CloseHandle(ProcessInfo.hProcess);
		if (ProcessInfo.hThread)
			CloseHandle(ProcessInfo.hThread);
		ProcessInfo.hProcess = NULL;
		ProcessInfo.hThread = NULL;
	}
	if (hEvent)
		CloseHandle(hEvent);
	if (hEventcad)
		CloseHandle(hEventcad);
	if (hEventPreConnect)
		CloseHandle(hEventPreConnect);
	if (hEndSessionEvent)
		CloseHandle(hEndSessionEvent);
	if (data)
		UnmapViewOfFile(data);
	if (hMapFile != NULL)
		CloseHandle(hMapFile);
}

BOOL UltraVNCService::LaunchProcessWin(DWORD dwSessionId, bool preconnect, bool rdpselect) {
	if (IsShutdown)
		return false;
	BOOL                 bReturn = FALSE;
	HANDLE               hToken = NULL;
	STARTUPINFO          StartUPInfo;
	PVOID                lpEnvironment = NULL;
	static int counter = 0;

	ZeroMemory(&StartUPInfo, sizeof(STARTUPINFO));
	ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));
	StartUPInfo.wShowWindow = SW_SHOW;
	//StartUPInfo.lpDesktop = "Winsta0\\Winlogon";
	StartUPInfo.lpDesktop = "Winsta0\\Default";
	StartUPInfo.cb = sizeof(STARTUPINFO);
	SetTBCPrivileges();
	createWinvncExeCall(preconnect, rdpselect);

	if (GetSessionUserTokenWin(&hToken, dwSessionId)) {
		if (CreateEnvironmentBlock(&lpEnvironment, hToken, FALSE)) {
			SetLastError(0);
			if (IsShutdown)
				return false;
			if (CreateProcessAsUser(hToken, NULL, app_path, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS, lpEnvironment, NULL, &StartUPInfo, &ProcessInfo)) {
				counter = 0;
				bReturn = TRUE;
				DWORD error = GetLastError();
#ifdef _DEBUG
				char			szText[256];
				sprintf_s(szText, " ++++++ CreateProcessAsUser winlogon %d\n", error);
				OutputDebugString(szText);
#endif
			}
			else
			{
				DWORD error = GetLastError();
#ifdef _DEBUG
				char			szText[256];
				sprintf_s(szText, " ++++++ CreateProcessAsUser failed %d %d %d\n", error, kickrdp, counter);
				OutputDebugString(szText);
#endif
				if (error == 233 && kickrdp == 1) {
					counter++;
					if (counter > 3) {
						typedef BOOLEAN(WINAPI* pWinStationConnect) (HANDLE, ULONG, ULONG, PCWSTR, ULONG);
						HMODULE  hlibwinsta = LoadLibrary("winsta.dll");
						pWinStationConnect WinStationConnectF = NULL;
						if (hlibwinsta)
							WinStationConnectF = (pWinStationConnect)GetProcAddress(hlibwinsta, "WinStationConnectW");
						if (WinStationConnectF != NULL) {
							DWORD ID = WTSGetActiveConsoleSessionId();
							WinStationConnectF(0, 0, ID, L"", 0);
							LockWorkStation();
						}
						Sleep(3000);
					}
				}
				else if (error == 233) {
					CreateRemoteSessionProcess(dwSessionId, true, hToken, NULL, app_path, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS, lpEnvironment, NULL, &StartUPInfo, &ProcessInfo);
					counter = 0;
					bReturn = TRUE;
				}
			}

			if (lpEnvironment)
				DestroyEnvironmentBlock(lpEnvironment);
		}//createenv
		else {
			SetLastError(0);
			if (IsShutdown)
				return false;
			if (CreateProcessAsUser(hToken, NULL, app_path, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo)) {
				counter = 0;
				bReturn = TRUE;
				DWORD error = GetLastError();
#ifdef _DEBUG
				char			szText[256];
				sprintf_s(szText, " ++++++ CreateProcessAsUser winlogon %d\n", error);
				OutputDebugString(szText);
#endif
			}
			else {
				DWORD error = GetLastError();
#ifdef _DEBUG
				char			szText[256];
				sprintf_s(szText, " ++++++ CreateProcessAsUser no env failed %d\n", error);
				OutputDebugString(szText);
#endif
				//Little trick needed, FUS sometimes has an unreachable logon session.
				 //Switch to USER B, logout user B
				 //The logon session is then unreachable
				 //We force the logon session on the console
				if (error == 233 && kickrdp == 1) {
					counter++;
					if (counter > 3) {
						typedef BOOLEAN(WINAPI* pWinStationConnect) (HANDLE, ULONG, ULONG, PCWSTR, ULONG);
						HMODULE  hlibwinsta = LoadLibrary("winsta.dll");
						pWinStationConnect WinStationConnectF = NULL;
						if (hlibwinsta)
							WinStationConnectF = (pWinStationConnect)GetProcAddress(hlibwinsta, "WinStationConnectW");

						if (WinStationConnectF != NULL) {
							DWORD ID = WTSGetActiveConsoleSessionId();
							WinStationConnectF(0, 0, ID, L"", 0);
							LockWorkStation();
						}
						Sleep(3000);
					}
				}
				else if (error == 233) {
					CreateRemoteSessionProcess(dwSessionId, true, hToken, NULL, app_path, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo);
					counter = 0;
					bReturn = TRUE;
				}
			}
		}  //getsession
		CloseHandle(hToken);
	}
	return bReturn;
}

BOOL UltraVNCService::CreateRemoteSessionProcess(
	IN DWORD        dwSessionId,
	IN BOOL         bUseDefaultToken,
	IN HANDLE       hToken,
	IN LPCWSTR      lpApplicationName,
	IN LPSTR       A_lpCommandLine,
	IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
	IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
	IN BOOL bInheritHandles,
	IN DWORD dwCreationFlags,
	IN LPVOID lpEnvironment,
	IN LPCWSTR lpCurrentDirectory,
	IN LPSTARTUPINFO A_lpStartupInfo,
	OUT LPPROCESS_INFORMATION lpProcessInformation)
{
	if (IsShutdown)
		return false;
	WCHAR       lpCommandLine[255];
	STARTUPINFOW StartupInfo;
	Char2Wchar(lpCommandLine, A_lpCommandLine, 255);
	ZeroMemory(&StartupInfo, sizeof(STARTUPINFOW));
	StartupInfo.wShowWindow = SW_SHOW;
	StartupInfo.lpDesktop = L"Winsta0\\Winlogon";
	StartupInfo.cb = sizeof(STARTUPINFOW);

	WCHAR           szWinStaPath[MAX_PATH];
	BOOL            bGetNPName = FALSE;
	WCHAR           szNamedPipeName[MAX_PATH] = L"";
	DWORD           dwNameLen;
	HINSTANCE       hInstWinSta;
	HANDLE          hNamedPipe;
	LPVOID          pData = NULL;
	BOOL            bRet = FALSE;
	DWORD           cbReadBytes, cbWriteBytes;
	DWORD           dwEnvLen = 0;
	union { CPAU_PARAM      cpauData; BYTE            bDump[0x2000]; };
	CPAU_RET_PARAM  cpauRetData;
	DWORD                   dwUsedBytes = sizeof(cpauData);
	LPBYTE                  pBuffer = (LPBYTE)(&cpauData + 1);
	GetSystemDirectoryW(szWinStaPath, MAX_PATH);
	lstrcatW(szWinStaPath, L"\\winsta.dll");
	hInstWinSta = LoadLibraryW(szWinStaPath);

	if (hInstWinSta) {
		pWinStationQueryInformationW pfWinStationQueryInformationW = (pWinStationQueryInformationW)GetProcAddress(hInstWinSta, "WinStationQueryInformationW");
		if (pfWinStationQueryInformationW)
			bGetNPName = pfWinStationQueryInformationW(0, dwSessionId, 0x21, szNamedPipeName, sizeof(szNamedPipeName), &dwNameLen);
		FreeLibrary(hInstWinSta);
	}
	if (!bGetNPName || szNamedPipeName[0] == '\0')
		swprintf(szNamedPipeName, 260, L"\\\\.\\Pipe\\TerminalServer\\SystemExecSrvr\\%d", dwSessionId);

	do {
		hNamedPipe = CreateFileW(szNamedPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
		if (hNamedPipe == INVALID_HANDLE_VALUE) {
			if (GetLastError() == ERROR_PIPE_BUSY) {
				if (!WaitNamedPipeW(szNamedPipeName, 30000))
					return FALSE;
			}
			else
				return FALSE;
		}
	} while (hNamedPipe == INVALID_HANDLE_VALUE);

	memset(&cpauData, 0, sizeof(cpauData));
	cpauData.bInheritHandles = bInheritHandles;
	cpauData.bUseDefaultToken = bUseDefaultToken;
	cpauData.dwCreationFlags = dwCreationFlags;
	cpauData.dwProcessId = GetCurrentProcessId();
	cpauData.hToken = hToken;
	cpauData.lpApplicationName = (LPWSTR)MarshallString(lpApplicationName, &cpauData, sizeof(bDump), &pBuffer, &dwUsedBytes);
	cpauData.lpCommandLine = (LPWSTR)MarshallString(lpCommandLine, &cpauData, sizeof(bDump), &pBuffer, &dwUsedBytes);
	cpauData.StartupInfo = StartupInfo;
	cpauData.StartupInfo.lpDesktop = (LPWSTR)MarshallString(cpauData.StartupInfo.lpDesktop, &cpauData, sizeof(bDump), &pBuffer, &dwUsedBytes);
	cpauData.StartupInfo.lpTitle = (LPWSTR)MarshallString(cpauData.StartupInfo.lpTitle, &cpauData, sizeof(bDump), &pBuffer, &dwUsedBytes);

	if (lpEnvironment) {
		if (dwCreationFlags & CREATE_UNICODE_ENVIRONMENT) {
			while ((dwEnvLen + dwUsedBytes <= sizeof(bDump))) {
				if (((LPWSTR)lpEnvironment)[dwEnvLen / 2] == '\0' && ((LPWSTR)lpEnvironment)[dwEnvLen / 2 + 1] == '\0') {
					dwEnvLen += 2 * sizeof(WCHAR);
					break;
				}
				dwEnvLen += sizeof(WCHAR);
			}
		}
		else {
			while (dwEnvLen + dwUsedBytes <= sizeof(bDump)) {
				if (((LPSTR)lpEnvironment)[dwEnvLen] == '\0' && ((LPSTR)lpEnvironment)[dwEnvLen + 1] == '\0') {
					dwEnvLen += 2;
					break;
				}
				dwEnvLen++;
			}
		}
		if (dwEnvLen + dwUsedBytes <= sizeof(bDump)) {
			memmove(pBuffer, lpEnvironment, dwEnvLen);
			cpauData.lpEnvironment = (LPVOID)dwUsedBytes;
			pBuffer += dwEnvLen;
			dwUsedBytes += dwEnvLen;
		}
		else
			cpauData.lpEnvironment = NULL;
	}
	else
		cpauData.lpEnvironment = NULL;
	cpauData.cbSize = dwUsedBytes;

	HANDLE hProcess = NULL;
	if (WriteFile(hNamedPipe, &cpauData, cpauData.cbSize, &cbWriteBytes, NULL)) {
		Sleep(250);
		if (ReadFile(hNamedPipe, &cpauRetData, sizeof(cpauRetData), &cbReadBytes, NULL)) {
			hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, cpauRetData.ProcInfo.dwProcessId);
#ifdef _DEBUG
			char			szText[256];
			sprintf_s(szText, " ++++++cpau  %i  %p %i %i %p %p\n", cpauRetData.bRetValue, cpauRetData.ProcInfo.hProcess, cpauRetData.ProcInfo.dwProcessId, cpauRetData.ProcInfo.dwThreadId, cpauRetData.ProcInfo.hThread, hProcess);
			OutputDebugString(szText);
#endif
			bRet = cpauRetData.bRetValue;
			if (bRet)
				*lpProcessInformation = cpauRetData.ProcInfo;
			else
				SetLastError(cpauRetData.dwLastErr);
		}
	}
	else
		bRet = FALSE;
	// function sometimes fail, the use processid to get hprocess... bug MS
	if (lpProcessInformation->hProcess == 0) lpProcessInformation->hProcess = hProcess;
	//this should never happen, looping connections
	if (lpProcessInformation->hProcess == 0)
		Sleep(5000);
	CloseHandle(hNamedPipe);
	return bRet;
}

DWORD UltraVNCService::MarshallString(LPCWSTR    pszText, LPVOID, DWORD  dwMaxSize, LPBYTE*
	ppNextBuf, DWORD* pdwUsedBytes)
{
	DWORD   dwOffset = *pdwUsedBytes;
	if (!pszText)
		return 0;
	DWORD   dwLen = (DWORD)(wcslen(pszText) + 1) * sizeof(WCHAR);
	if (*pdwUsedBytes + dwLen > dwMaxSize)
		return 0;
	memmove(*ppNextBuf, pszText, dwLen);
	*pdwUsedBytes += dwLen;
	*ppNextBuf += dwLen;
	return dwOffset;

}

BOOL UltraVNCService::Char2Wchar(WCHAR* pDest, char* pSrc, int nDestStrLen) 
{
	int nSrcStrLen = 0;
	int nOutputBuffLen = 0;
	int retcode = 0;

	if (pDest == NULL || pSrc == NULL)
	{
		return FALSE;
	}

	nSrcStrLen = (int)strlen(pSrc);
	if (nSrcStrLen == 0)
	{
		return FALSE;
	}

	nDestStrLen = nSrcStrLen;

	if (nDestStrLen > MAXSTRLENGTH - 1)
	{
		return FALSE;
	}
	memset(pDest, 0, sizeof(TCHAR) * nDestStrLen);
	nOutputBuffLen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pSrc, nSrcStrLen, pDest, nDestStrLen);

	if (nOutputBuffLen == 0)
	{
		retcode = GetLastError();
		return FALSE;
	}

	pDest[nOutputBuffLen] = '\0';
	return TRUE;
}

BOOL UltraVNCService::get_winlogon_handle(OUT LPHANDLE  lphUserToken, DWORD mysessionID)
{
	BOOL   bResult = FALSE;
	HANDLE hProcess;
	HANDLE hAccessToken = NULL;
	HANDLE hTokenThis = NULL;
	DWORD ID_session = 0;
	ID_session = mysessionID;
	DWORD Id = 0;
	Id = Find_winlogon(ID_session);
	if (Id == -1)
		Id = GetwinlogonPid();

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Id);
	if (hProcess) {
		OpenProcessToken(hProcess, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, &hTokenThis);
		bResult = DuplicateTokenEx(hTokenThis, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenPrimary, lphUserToken);
		SetTokenInformation(*lphUserToken, TokenSessionId, &ID_session, sizeof(DWORD));
		CloseHandle(hTokenThis);
		CloseHandle(hProcess);
	}
	return bResult;
}

BOOL UltraVNCService::GetSessionUserTokenWin(OUT LPHANDLE  lphUserToken, DWORD mysessionID)
{
	BOOL   bResult = FALSE;
	if (lphUserToken != NULL) {
		bResult = get_winlogon_handle(lphUserToken, mysessionID);
	}
	return bResult;
}

DWORD UltraVNCService::GetwinlogonPid()
{
	DWORD dwExplorerLogonPid = 0;
	PROCESSENTRY32 procEntry;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
		return 0;

	procEntry.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnap, &procEntry)) {
		CloseHandle(hSnap);
		return 0;
	}

	do {
		if (_stricmp(procEntry.szExeFile, "winlogon.exe") == 0)
			dwExplorerLogonPid = procEntry.th32ProcessID;

	} while (!IsShutdown && Process32Next(hSnap, &procEntry));
	CloseHandle(hSnap);
	return dwExplorerLogonPid;
}

DWORD UltraVNCService::Find_winlogon(DWORD SessionId)
{
	PWTS_PROCESS_INFO pProcessInfo = NULL;
	DWORD         ProcessCount = 0;
	DWORD         Id = -1;

	typedef BOOL(WINAPI* pfnWTSEnumerateProcesses)(HANDLE, DWORD, DWORD, PWTS_PROCESS_INFO*, DWORD*);
	typedef VOID(WINAPI* pfnWTSFreeMemory)(PVOID);

	helper::DynamicFn<pfnWTSEnumerateProcesses> pWTSEnumerateProcesses("wtsapi32", "WTSEnumerateProcessesA");
	helper::DynamicFn<pfnWTSFreeMemory> pWTSFreeMemory("wtsapi32", "WTSFreeMemory");

	if (pWTSEnumerateProcesses.isValid() && pWTSFreeMemory.isValid()) {
		if ((*pWTSEnumerateProcesses)(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo, &ProcessCount)) {
			// dump each process description
			for (DWORD CurrentProcess = 0; CurrentProcess < ProcessCount; CurrentProcess++) {
				if (_stricmp(pProcessInfo[CurrentProcess].pProcessName, "winlogon.exe") == 0) {
					if (SessionId == pProcessInfo[CurrentProcess].SessionId) {
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


BOOL UltraVNCService::SetTBCPrivileges(VOID) {
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

void UltraVNCService::wait_for_existing_process()
{
	while (!IsShutdown && (hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Global\\SessionEventUltra")) != NULL) {
		SetEvent(hEvent); // signal tray icon to shut down 
		CloseHandle(hEvent);
		Sleep(1000);
	}
}

bool UltraVNCService::IsSessionStillActive(int ID)
{
	typedef BOOL(WINAPI* pfnWTSEnumerateSessions)(HANDLE, DWORD, DWORD, PWTS_SESSION_INFO*, DWORD*);;
	typedef VOID(WINAPI* pfnWTSFreeMemory)(PVOID);

	helper::DynamicFn<pfnWTSEnumerateSessions> pWTSEnumerateSessions("wtsapi32", "WTSEnumerateSessionsA");
	helper::DynamicFn<pfnWTSFreeMemory> pWTSFreeMemory("wtsapi32", "WTSFreeMemory");
	if (pWTSEnumerateSessions.isValid() && pWTSFreeMemory.isValid()) {
		WTS_SESSION_INFO* pSessions = 0;
		DWORD   nSessions(0);
		DWORD   rdpSessionExists = false;
		if ((*pWTSEnumerateSessions)(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessions, &nSessions)) {
			for (DWORD i(0); i < nSessions && !rdpSessionExists; ++i) {
				//exclude console session
				if ((_stricmp(pSessions[i].pWinStationName, "Console") == 0) && (pSessions[i].SessionId == ID))
					rdpSessionExists = true;
				else if ((pSessions[i].SessionId == ID) && (pSessions[i].State == WTSActive || pSessions[i].State == WTSShadow || pSessions[i].State == WTSConnectQuery))
					rdpSessionExists = true;
			}
			(*pWTSFreeMemory)(pSessions);
		}
		return rdpSessionExists ? true : false;
	}
	return false;
}
#endif
