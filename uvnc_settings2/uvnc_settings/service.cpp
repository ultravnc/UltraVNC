#include "stdafx.h"
#include "resource.h"
#include <time.h>
#include "upnp.h"
#include "firewall.h"
#include "log.h"
#include <iphlpapi.h>
#pragma comment ( lib, "iphlpapi" )
#include <shlwapi.h>
#pragma comment ( lib, "shlwapi" )
#ifndef _WINSOCK2API_
	#include <winsock2.h>
	#pragma comment(lib, "ws2_32.lib")
#endif

static char service_path[MAX_PATH];
static char service_file[MAX_PATH];
char service_name[256]="uvnc_service";
char *app_name = "UltraVNC";
#define VNCDEPENDENCIES    "Tcpip\0\0"


////////////////////////////////////////////////////////////////////////////////
void set_service_description()
{
    // Add service description 
	DWORD	dw;
	HKEY hKey;
	char tempName[256];
    char desc[] = "Provides secure remote desktop sharing";
	_snprintf(tempName,  sizeof tempName, "SYSTEM\\CurrentControlSet\\Services\\%s", service_name);
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
////////////////////////////////////////////////////////////////////////////////
static int pad()
{
	char exe_file_name[MAX_PATH], dir[MAX_PATH], *ptr;
    GetModuleFileName(0, exe_file_name, MAX_PATH);

    /* set current directory */
    strcpy(dir, exe_file_name);
    ptr=strrchr(dir, '\\'); /* last backslash */
    if(ptr)
        ptr[1]='\0'; /* truncate program name */
    if(!SetCurrentDirectory(dir)) {
        return 1;
    }

    strcpy(service_path, "\"");
    strcat(service_path, dir);
	strcat(service_path, "\winvnc.exe");
    strcat(service_path, "\" -service");

    strcpy(service_file, dir);
	strcat(service_file, "\winvnc.exe");
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
DWORD MessageBoxSecure(HWND hWnd,LPCTSTR lpText,LPCTSTR lpCaption,UINT uType)
{
	DWORD retunvalue;
	retunvalue=MessageBox(hWnd,lpText,lpCaption,uType);
	return retunvalue;
}
////////////////////////////////////////////////////////////////////////////////
bool existFile(char *mychar)
{
	DWORD value1=GetFileAttributes(mychar);
	if (value1== INVALID_FILE_ATTRIBUTES)
		{
			DWORD errnr=GetLastError();
			return false;
		}
    return true;
}
////////////////////////////////////////////////////////////////////////////////
int install_service(void) {

    SC_HANDLE scm, service;
	pad();
	if (!existFile(service_file))
	{
		 MessageBoxSecure(NULL, "Verify that winvnc.exe and uvnc_settings are in the same folder\n can not create the uvnc_service",
            app_name, MB_ICONERROR);
		return 1;
	}
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
			//MessageBoxSecure(NULL, "Failed: Already exist",
            //"UltraVNC", MB_ICONERROR);
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
int uninstall_service(void) {
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
        //MessageBoxSecure(NULL, "The service is still running, disable it first",
        //    "UltraVNC", MB_ICONERROR);
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
bool s_installed=0;
bool s_running=0;

void
Real_start_service()
{
    char command[MAX_PATH + 32]; // 29 January 2008 jdp
    _snprintf(command, sizeof command, "net start \"%s\"", service_name);
	WinExec(command,SW_HIDE);
}

void
Real_stop_service()
{
    char command[MAX_PATH + 32]; // 29 January 2008 jdp
    _snprintf(command, sizeof command, "net stop \"%s\"", service_name);
	WinExec(command,SW_HIDE);
}

bool Is_Service_running()
{
	bool value;

	return value;
}


bool IsInstalled()
{
	s_running=false;

    BOOL bResult = FALSE;
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ENUMERATE_SERVICE); // full access
    if (hSCM) {
        SC_HANDLE hService = ::OpenService(hSCM,
                                           service_name,
                                           SERVICE_ALL_ACCESS);
        if (hService) {
            bResult = TRUE;
			SERVICE_STATUS_PROCESS ssp;
			DWORD dwBytesNeeded;
			if ( QueryServiceStatusEx( hService, SC_STATUS_PROCESS_INFO,(LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS),&dwBytesNeeded ) !=0 )
			{	
				if ( ssp.dwCurrentState == SERVICE_RUNNING )
					{
						s_running=true;
					}


			}
			else 
			{
				DWORD aa= GetLastError() ;
				aa++;
			}

            ::CloseServiceHandle(hService);
        }
        ::CloseServiceHandle(hSCM);
    }
    return (FALSE != bResult);
}


BOOL CALLBACK DlgProcService(HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) {
		
	case WM_INITDIALOG: 
		{		
			SetTimer(hwnd,1001,5000,NULL);
			pad();
			s_installed=IsInstalled();
			if (!s_installed) EnableWindow(GetDlgItem(hwnd, IDC_INSTALSS), TRUE);
			else EnableWindow(GetDlgItem(hwnd, IDC_INSTALSS), FALSE);
			if (s_installed) EnableWindow(GetDlgItem(hwnd, IDC_UNINSTALLS),TRUE );
			else EnableWindow(GetDlgItem(hwnd, IDC_UNINSTALLS), FALSE);
			if (!s_running && s_installed) EnableWindow(GetDlgItem(hwnd, IDC_STARTS), TRUE);
			else EnableWindow(GetDlgItem(hwnd, IDC_STARTS), FALSE);
			if (s_running)EnableWindow(GetDlgItem(hwnd, IDC_STOPS), TRUE);
			else EnableWindow(GetDlgItem(hwnd, IDC_STOPS), FALSE);
			return TRUE;
		}

	case WM_TIMER:
		if (wParam==1001) 
		{
			s_installed=IsInstalled();
			if (!s_installed) EnableWindow(GetDlgItem(hwnd, IDC_INSTALSS), TRUE);
			else EnableWindow(GetDlgItem(hwnd, IDC_INSTALSS), FALSE);
			if (s_installed) EnableWindow(GetDlgItem(hwnd, IDC_UNINSTALLS),TRUE );
			else EnableWindow(GetDlgItem(hwnd, IDC_UNINSTALLS), FALSE);
			if (!s_running && s_installed) EnableWindow(GetDlgItem(hwnd, IDC_STARTS), TRUE);
			else EnableWindow(GetDlgItem(hwnd, IDC_STARTS), FALSE);
			if (s_running)EnableWindow(GetDlgItem(hwnd, IDC_STOPS), TRUE);
			else EnableWindow(GetDlgItem(hwnd, IDC_STOPS), FALSE);		
		}
	
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{
		case IDC_HELP2:
			if (lParam==6)
			
{
			char link[256];
			strcpy(link,"http://www.uvnc.com/webhelp/");
			strcat(link,"service");
			strcat(link,".html");
			ShellExecute(GetDesktopWindow(), "open", link, "", 0, SW_SHOWNORMAL);
			}
			break;
		case IDOK:
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		case IDC_INSTALSS:
			install_service();
			break;
		case IDC_UNINSTALLS:
			Real_stop_service();
			Sleep(2000);
			uninstall_service();
			break;
		case IDC_STARTS:
			Real_start_service();
			break;
		case IDC_STOPS:
			Real_stop_service();
			break;

		}
		return 0;	
	}

	return 0;
}