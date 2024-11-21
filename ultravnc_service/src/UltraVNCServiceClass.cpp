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
#include "framework.h"
#include "UltraVNCServiceClass.h"
#include <wtsapi32.h>
#include <userenv.h>
#include <thread>
#include <string>
#include "../../common/win32_helpers.h"
#include "../../common/inifile.h"
#include "resource.h"

SERVICE_STATUS UltraVNCService::serviceStatus{};
SERVICE_STATUS_HANDLE UltraVNCService::serviceStatusHandle = NULL;
char UltraVNCService::service_path[MAX_PATH]{};
char UltraVNCService::service_name[256] = "ultravnc_server";
char UltraVNCService::display_name[256] = "UltraVNC Server";
char UltraVNCService::description[256] = "UltraVNC Server enables remote connectivity to this machine using the VNC/RFB protocol.";
char UltraVNCService::ultravnc_server_ui[256] = "\\ultravnc_server.exe";
char UltraVNCService::app_path[MAX_PATH]{};
char UltraVNCService::app_path_UI[MAX_PATH]{};
int UltraVNCService::kickrdp = 0;
PROCESS_INFORMATION  UltraVNCService::ProcessInfo{};
bool UltraVNCService::IsShutdown = false;
HANDLE UltraVNCService::hEndSessionEvent = NULL;
HANDLE UltraVNCService::hEvent = NULL;
int UltraVNCService::clear_console = 0;
const char* UltraVNCService::app_name = "UltraVNC";
char  UltraVNCService::cmdtext[256]{};
IniFile myIniFile;
bool UltraVNCService::preconnect_start = false;

extern HINSTANCE g_hInstance;

UltraVNCService::UltraVNCService()
{
	
}
////////////////////////////////////////////////////////////////////////////////
void yesUVNCMessageBox(HWND m_hWnd, const char* body, const char* szHeader, int icon)
{
	wchar_t w_header[128];
	wchar_t w_body[1024];
	size_t outSize;

	mbstowcs_s(&outSize, w_header, szHeader, strlen(szHeader) + 1);
	mbstowcs_s(&outSize, w_body, body, strlen(body) + 1);

	HRESULT hr;
	TASKDIALOGCONFIG tdc = { sizeof(TASKDIALOGCONFIG) };
	int nClickedBtn = 0;
	LPCWSTR szTitle = L"UltraVNC Server - Service";


	tdc.cbSize = sizeof(tdc);
	tdc.hInstance = g_hInstance;
	tdc.hwndParent = m_hWnd;
	tdc.dwCommonButtons = TDCBF_OK_BUTTON;
	tdc.pszWindowTitle = szTitle;
	tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
	tdc.pszMainIcon = MAKEINTRESOURCEW(IDI_ICON1);// TD_INFORMATION_ICON;
	tdc.pszMainInstruction = w_header;
	tdc.pszContent = w_body;

	hr = TaskDialogIndirect(&tdc, &nClickedBtn, NULL, NULL);
};

#pragma comment(lib, "Version.lib")
wchar_t* GetVersionFromResource(wchar_t* version)
{
	HRSRC hResInfo;
	DWORD dwSize;
	HGLOBAL hResData;
	LPVOID pRes, pResCopy;
	UINT uLen = 0;
	VS_FIXEDFILEINFO* lpFfi = NULL;
	HINSTANCE hInst = ::GetModuleHandle(NULL);

	hResInfo = FindResource(hInst, MAKEINTRESOURCE(1), RT_VERSION);
	if (hResInfo)
	{
		dwSize = SizeofResource(hInst, hResInfo);
		hResData = LoadResource(hInst, hResInfo);
		if (hResData)
		{
			pRes = LockResource(hResData);
			if (pRes)
			{
				pResCopy = LocalAlloc(LMEM_FIXED, dwSize);
				if (pResCopy)
				{
					CopyMemory(pResCopy, pRes, dwSize);

					if (VerQueryValue(pResCopy, ("\\"), (LPVOID*)&lpFfi, &uLen))
					{
						if (lpFfi != NULL)
						{
							DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
							DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;

							DWORD dwLeftMost = HIWORD(dwFileVersionMS);
							DWORD dwSecondLeft = LOWORD(dwFileVersionMS);
							DWORD dwSecondRight = HIWORD(dwFileVersionLS);
							DWORD dwRightMost = LOWORD(dwFileVersionLS);

							swprintf_s(version, 128, L"%d.%d.%d.%d", dwLeftMost, dwSecondLeft, dwSecondRight, dwRightMost);
						}
					}

					LocalFree(pResCopy);
				}
			}
		}
	}
	wcscat_s(version, 128, (wchar_t*)L"-dev");
	return version;
}

void updateButtons(HWND hwnd)
{
	if (!UltraVNCService::IsServiceInstalled()) {
		SendMessage(hwnd, TDM_ENABLE_BUTTON, 1000, TRUE);
		SendMessage(hwnd, TDM_ENABLE_BUTTON, 1001, FALSE);
		SendMessage(hwnd, TDM_ENABLE_BUTTON, 1002, FALSE);
		SendMessage(hwnd, TDM_ENABLE_BUTTON, 1003, FALSE);
	}

	else if (UltraVNCService::IsServiceInstalled() && UltraVNCService::IsServiceRunning()) {
		SendMessage(hwnd, TDM_ENABLE_BUTTON, 1000, FALSE);
		SendMessage(hwnd, TDM_ENABLE_BUTTON, 1001, FALSE);
		SendMessage(hwnd, TDM_ENABLE_BUTTON, 1002, TRUE);
		SendMessage(hwnd, TDM_ENABLE_BUTTON, 1003, FALSE);
	}
	else if (UltraVNCService::IsServiceInstalled() && !UltraVNCService::IsServiceRunning()) {
		SendMessage(hwnd, TDM_ENABLE_BUTTON, 1000, FALSE);
		SendMessage(hwnd, TDM_ENABLE_BUTTON, 1001, TRUE);
		SendMessage(hwnd, TDM_ENABLE_BUTTON, 1002, FALSE);
		SendMessage(hwnd, TDM_ENABLE_BUTTON, 1003, TRUE);
	}
}

int  StartUVNCMessageBox(HWND m_hWnd, const char* body, const char* szHeader, int icon)
{
	wchar_t w_header[128];
	wchar_t w_body[1024];
	wchar_t w_version[128];
	size_t outSize;

	mbstowcs_s(&outSize, w_header, szHeader, strlen(szHeader) + 1);
	mbstowcs_s(&outSize, w_body, body, strlen(body) + 1);

	GetVersionFromResource(w_version);

	HRESULT hr;
	TASKDIALOGCONFIG tdc = { sizeof(TASKDIALOGCONFIG) };
	int nClickedBtn = 0;
	wchar_t szTitle[2048] = L"UltraVNC Server - Service Manager";

	TASKDIALOG_BUTTON aCustomButtons[] = {
	   { 1000, L"Install"},
	   { 1001, L"Uninstall"},
	   { 1002, L"Stop"},
	   { 1003, L"Start"}
	};

	tdc.cbSize = sizeof(tdc);
	tdc.hInstance = g_hInstance;
	tdc.hwndParent = m_hWnd;
	tdc.pszWindowTitle = szTitle;
	//tdc.dwCommonButtons = TDCBF_CANCEL_BUTTON;
	tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS | TDF_ENABLE_HYPERLINKS | TDF_CALLBACK_TIMER;
	tdc.pButtons = aCustomButtons;
	tdc.cButtons = _countof(aCustomButtons);
	tdc.pszMainIcon = MAKEINTRESOURCEW(IDI_ICON1);// TD_INFORMATION_ICON;
	//tdc.pszMainInstruction = w_header;
	tdc.pszContent = w_body;
	wchar_t footer[4000]{};
	wcscpy_s(footer, L"UltraVNC Server - Service Manager -");
	wcscat_s(footer, w_version);
#ifdef _X64
	wcscat_s(footer, L" - x64");
#else
	wcscat_s(footer, L" - x86");
#endif

	wcscat_s(footer, L"\nCopyright © 2002-2025 UltraVNC Team Members\n<a href=\"https://uvnc.com\">Website</a> | <a href=\"https://forum.uvnc.com\">Forum</a>");
	tdc.pszFooter = footer;
	tdc.pfCallback = [](HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR lpRefData) -> HRESULT {
		static int elapsedTime = 0;
		if (uNotification == TDN_TIMER) {
			// Timer notification: wParam contains the elapsed time in milliseconds
			elapsedTime = static_cast<int>(wParam / 1000);
			if (elapsedTime >= 3) {
				updateButtons(hwnd);
				elapsedTime = 0;
			}			
		}

		if (uNotification == TDN_HYPERLINK_CLICKED) {
			LPCWSTR url = (LPCWSTR)lParam;
			ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
		}
		if (uNotification == TDN_CREATED) {
			updateButtons(hwnd);
		}
		if (uNotification == TDN_BUTTON_CLICKED) {
			char command[MAX_PATH + 32]{};
			bool handled = false;
			switch (wParam) {
			case 1000: UltraVNCService::install_service(); handled = true; break;
			case 1001: UltraVNCService::uninstall_service(); handled = true; break;
			case 1002:
				_snprintf_s(command, sizeof command, "net stop \"%s\"", UltraVNCService::service_name);
				WinExec(command, SW_HIDE);
				handled = true; 
				Sleep(4000);
				break;
			case 1003:
				_snprintf_s(command, sizeof command, "net start \"%s\"", UltraVNCService::service_name);
				WinExec(command, SW_HIDE);
				Sleep(4000);
				handled = true; 
				break;
			}			
			updateButtons(hwnd);
			return handled ? S_FALSE: S_OK ;
		}

		return S_OK;
	};

	hr = TaskDialogIndirect(&tdc, &nClickedBtn, NULL, NULL);
	if (SUCCEEDED(hr))
		return nClickedBtn;
	return 0;
};
////////////////////////////////////////////////////////////////////////////////
void WINAPI UltraVNCService::service_main(DWORD argc, LPTSTR* argv) {
	serviceStatus.dwServiceType = SERVICE_WIN32;
	serviceStatus.dwCurrentState = SERVICE_STOPPED;
	serviceStatus.dwControlsAccepted = 0;
	serviceStatus.dwWin32ExitCode = NO_ERROR;
	serviceStatus.dwServiceSpecificExitCode = NO_ERROR;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;

	serviceStatusHandle = RegisterServiceCtrlHandlerExA(service_name, control_handler_ex, 0);
	if (!serviceStatusHandle) {
		// Log error and return
		OutputDebugStringA("Failed to register service control handler.");
		return;
	}

	serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	SetServiceStatus(serviceStatusHandle, &serviceStatus);

	serviceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
	serviceStatus.dwControlsAccepted |= SERVICE_ACCEPT_SESSIONCHANGE | SERVICE_CONTROL_PRESHUTDOWN;
	serviceStatus.dwCurrentState = SERVICE_RUNNING;
	if (!SetServiceStatus(serviceStatusHandle, &serviceStatus)) {
		// Log error
		OutputDebugStringA("Failed to set service status to running.");
	}

	Restore_after_reboot();

	std::thread monitoringThread([] {
		while (serviceStatus.dwCurrentState == SERVICE_RUNNING) {
			monitorSessions();
			Sleep(3000);
		}
		});

	monitoringThread.join();

	serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
	SetServiceStatus(serviceStatusHandle, &serviceStatus);

	serviceStatus.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
	serviceStatus.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(serviceStatusHandle, &serviceStatus);
}
////////////////////////////////////////////////////////////////////////////////
void WINAPI UltraVNCService::control_handler(DWORD controlCode)
{
  control_handler_ex(controlCode, 0, 0, 0);
}
////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI UltraVNCService::control_handler_ex(DWORD controlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
	switch (controlCode) {
	case SERVICE_CONTROL_INTERROGATE:
		// No action needed for this code, just return
		break;

	case SERVICE_CONTROL_PRESHUTDOWN:
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP:
		// Set service state to STOP_PENDING as we're shutting down
		serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		// Signal end of session event and any additional cleanup
		SetEvent(hEndSessionEvent);
		SetEvent(hEvent);
		// Optional: Log that shutdown is occurring
		// Log("Service is stopping...");
		return NO_ERROR;

	case SERVICE_CONTROL_PAUSE:
		// Add code to handle the pause request
		// Log("Service is being paused...");
		break;

	case SERVICE_CONTROL_CONTINUE:
		// Add code to handle the continue request
		// Log("Service is resuming...");
		break;

	case SERVICE_CONTROL_SESSIONCHANGE:
		if (dwEventType == WTS_REMOTE_DISCONNECT) {
			// If the event indicates a remote disconnect, handle the session change
			if (clear_console) {
				// This method should handle disconnections from remote sessions
				disconnect_remote_sessions();
			}
		}
		break;

	default:
		// If the control code is a custom one, we don't handle it here
		if (controlCode >= 128 && controlCode <= 255) {
			// Custom control codes (nothing to do here)
			break;
		}
		else {
			// Invalid control code, handle accordingly (e.g., log)
			// Log("Unknown control code received: " + std::to_string(controlCode));
			break;
		}
	}

	// Update service status after handling the control event
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
void UltraVNCService::set_service_description() {
	// Registry key path
	std::string regPath = "SYSTEM\\CurrentControlSet\\Services\\" + std::string(service_name);

	HKEY hKey;
	DWORD dwDisposition;

	// Create or open the registry key
	LONG result = RegCreateKeyExA(
		HKEY_LOCAL_MACHINE,
		regPath.c_str(),
		0,
		NULL, // Class, not needed
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE, // Only need write access
		NULL,
		&hKey,
		&dwDisposition);

	if (result != ERROR_SUCCESS) {
		char errorMsg[256];
		snprintf(errorMsg, sizeof(errorMsg), "Failed to open or create registry key. Error code: %ld", result);
		MessageBoxA(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
		return;  // Exit if the operation failed
	}

	// Set the "Description" value in the registry key
	result = RegSetValueExA(
		hKey,
		"Description",
		0,
		REG_SZ,
		(const BYTE*)description,
		static_cast<DWORD>(strlen(description) + 1));

	if (result != ERROR_SUCCESS) {
		char errorMsg[256];
		snprintf(errorMsg, sizeof(errorMsg), "Failed to set registry value. Error code: %ld", result);
		MessageBoxA(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
	}
	else {
		//MessageBoxA(NULL, "Service description set successfully.", "Success", MB_ICONINFORMATION | MB_OK);
	}

	// Always close the registry key handle
	RegCloseKey(hKey);
}
////////////////////////////////////////////////////////////////////////////////
#define VNCDEPENDENCIES    "Tcpip\0EventLog\0\0"  // Example of dependencies
int UltraVNCService::install_service(bool show) {
	SC_HANDLE scm, service;
	// Assuming 'pad()' function is some sort of padding, call it if needed
	pad();

	// Open the Service Control Manager with permission to create services
	scm = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
	if (!scm) {
		yesUVNCMessageBox(NULL, "Failed to open service control manager", app_name, MB_ICONERROR);
		return 1;
	}

	// Create the service (service path should be a valid executable path)
	service = CreateService(scm, service_name, display_name, SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, service_path,
		NULL, NULL, VNCDEPENDENCIES, NULL, NULL);
	if (!service) {
		DWORD myerror = GetLastError();
		if (myerror == ERROR_ACCESS_DENIED) {
			yesUVNCMessageBox(NULL, "Failed: Permission denied", app_name, MB_ICONERROR);
			CloseServiceHandle(scm);
			return 1;
		}
		if (myerror == ERROR_SERVICE_EXISTS) {
			yesUVNCMessageBox(NULL, "Service already exists", app_name, MB_ICONWARNING);
			CloseServiceHandle(scm);
			return 1;
		}

		// Handle other errors
		yesUVNCMessageBox(NULL, "Failed to create a new service", app_name, MB_ICONERROR);
		CloseServiceHandle(scm);
		return 1;
	}
	else {
		if (show) {
			char Msg[256];
			snprintf(Msg, sizeof(Msg), "%s was correct installed", display_name);
			yesUVNCMessageBox(NULL, Msg, app_name, MB_ICONERROR);
		}
		// Set service description if creation succeeded
		set_service_description();
	}

	// Clean up
	CloseServiceHandle(service);
	CloseServiceHandle(scm);
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
int UltraVNCService::uninstall_service(void) {
	SC_HANDLE scm, service;
	SERVICE_STATUS aServiceStatus;
	const int MAX_RETRIES = 5;  // Maximum retries
	int retryCount = 0;

	scm = OpenSCManager(0, 0, SC_MANAGER_CONNECT);
	if (!scm) {
		yesUVNCMessageBox(NULL, "Failed to open service control manager", app_name, MB_ICONERROR);
		return 1;
	}

	service = OpenService(scm, service_name, SERVICE_QUERY_STATUS | DELETE);
	if (!service) {
		DWORD myerror = GetLastError();
		if (myerror == ERROR_ACCESS_DENIED) {
			yesUVNCMessageBox(NULL, "Failed: Permission denied", app_name, MB_ICONERROR);
			CloseServiceHandle(scm);
			return 1;
		}
		if (myerror == ERROR_SERVICE_DOES_NOT_EXIST) {
			// Service not installed, silently return
			CloseServiceHandle(scm);
			return 1;
		}

		yesUVNCMessageBox(NULL, "Failed to open the service", app_name, MB_ICONERROR);
		CloseServiceHandle(scm);
		return 1;
		}

	// Query the service status
	if (!QueryServiceStatus(service, &aServiceStatus)) {
		yesUVNCMessageBox(NULL, "Failed to query service status", app_name, MB_ICONERROR);
		CloseServiceHandle(service);
		CloseServiceHandle(scm);
		return 1;
	}

	// If the service is not stopped, wait and retry
	while (aServiceStatus.dwCurrentState != SERVICE_STOPPED && retryCount < MAX_RETRIES) {
		retryCount++;
		Sleep(2500);  // Wait for 2.5 seconds before retrying
		if (!QueryServiceStatus(service, &aServiceStatus)) {
			yesUVNCMessageBox(NULL, "Failed to query service status during retry", app_name, MB_ICONERROR);
			CloseServiceHandle(service);
			CloseServiceHandle(scm);
			return 1;
		}
	}

	// If the service still isn't stopped after max retries, show an error
	if (aServiceStatus.dwCurrentState != SERVICE_STOPPED) {
		yesUVNCMessageBox(NULL, "Service failed to stop within the allowed time", app_name, MB_ICONERROR);
		CloseServiceHandle(service);
		CloseServiceHandle(scm);
		return 1;
	}

	// Delete the service
	if (!DeleteService(service)) {
		yesUVNCMessageBox(NULL, "Failed to delete the service", app_name, MB_ICONERROR);
		CloseServiceHandle(service);
		CloseServiceHandle(scm);
		return 1;
	}

	CloseServiceHandle(service);
	CloseServiceHandle(scm);
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
int UltraVNCService::pad() {
	char exe_file_name[MAX_PATH], dir[MAX_PATH], * ptr;

	// Get the full path of the executable
	GetModuleFileName(0, exe_file_name, MAX_PATH);

	// Set current directory to the directory of the executable
	strcpy_s(dir, exe_file_name);
	ptr = strrchr(dir, '\\'); // Find the last backslash to get the directory part
	if (ptr) {
		ptr[1] = '\0'; // Truncate the program name, leaving only the directory
	}

	// Set the current working directory to the directory of the executable
	if (!SetCurrentDirectory(dir)) {
		// Return 1 if setting the directory failed
		return 1;
	}

	// Prepare the service path, adding quotes around the executable path
	strcpy_s(service_path, "\"");
	strcat_s(service_path, exe_file_name);  // Add the executable's full path
	strcat_s(service_path, "\"");           // Close the quote
	strcat_s(service_path, " -service");    // Add the "-service" argument to indicate running as a service

	return 0;
}
////////////////////////////////////////////////////
BOOL UltraVNCService::DeleteServiceSafeBootKey()
{
	LONG lSuccess;
	char szKey[1024];
	_snprintf_s(szKey, 1024, "SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\%s\\%s", "Network", service_name);
	lSuccess = RegDeleteKey(HKEY_LOCAL_MACHINE, szKey);
	return lSuccess == ERROR_SUCCESS;

}
////////////////////////////////////////////////////
void UltraVNCService::Restore_safemode()
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
////////////////////////////////////////////////////
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
////////////////////////////////////////////////////
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
////////////////////////////////////////////////////
bool UltraVNCService::IsAnyRDPSessionActive()
{
	WTS_SESSION_INFO* pSessions = 0;
	DWORD   nSessions(0);
	DWORD   rdpSessionExists = false;

	if (WTSEnumerateSessionsA(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessions, &nSessions)) {
		for (DWORD i(0); i < nSessions && !rdpSessionExists; ++i) {
			if ((_stricmp(pSessions[i].pWinStationName, "Console") != 0) &&
				(pSessions[i].State == WTSActive ||
					pSessions[i].State == WTSShadow ||
					pSessions[i].State == WTSConnectQuery
					))
				rdpSessionExists = true;
		}
		WTSFreeMemory(pSessions);
		return rdpSessionExists ? true : false;
	}
	return false;
}
////////////////////////////////////////////////////
int UltraVNCService::createWinvncExeCall(bool preconnect, bool rdpselect)
{	
	char exe_file_name[MAX_PATH];
	char cmdline[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);
	// Extract the directory part of the path (remove the file name)
	char* last_backslash = strrchr(exe_file_name, '\\');
	if (last_backslash != NULL) {
		*last_backslash = '\0';  // Terminate the string at the last backslash
	}

	// Replace the filename with the new one (e.g., "new_executable.exe")
	strcpy_s(app_path_UI, MAX_PATH, exe_file_name); // Copy the path
	strcat_s(app_path_UI, MAX_PATH, ultravnc_server_ui); // Add the new file name
	if (preconnect)
		strcat_s(app_path_UI, " -preconnect");
	if (rdpselect)
		strcat_s(app_path_UI, " -service_rdp_run");
	else
		strcat_s(app_path_UI, " -service_run");
	IniFile myIniFile;
	kickrdp = myIniFile.ReadInt("admin", "kickrdp", kickrdp);
	clear_console = myIniFile.ReadInt("admin", "clearconsole", clear_console);
	myIniFile.ReadString("admin", "service_commandline", cmdline, 256);
	if (strlen(cmdline) != 0) {
		strcpy_s(app_path_UI, exe_file_name);
		if (preconnect)
			strcat_s(app_path_UI, " -preconnect");
		strcat_s(app_path_UI, " ");
		strcat_s(app_path_UI, cmdline);
		if (rdpselect)
			strcat_s(app_path_UI, " -service_rdp_run");
		else
			strcat_s(app_path_UI, " -service_run");
	}
	return 0;
}
////////////////////////////////////////////////////
void UltraVNCService::monitorSessions() {
	BOOL RDPMODE = false;	
	RDPMODE = myIniFile.ReadInt("admin", "rdpmode", 0);

	hEvent = CreateEvent(NULL, FALSE, FALSE, "Global\\SessionEventUltra");
	HANDLE hEventcad = CreateEvent(NULL, FALSE, FALSE, "Global\\SessionEventUltraCad");
	HANDLE hEventPreConnect = CreateEvent(NULL, FALSE, FALSE, "Global\\SessionEventUltraPreConnect");
	hEndSessionEvent = CreateEvent(NULL, FALSE, FALSE, "Global\\EndSessionEvent");
	HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), "Global\\SessionUltraPreConnect");

	if (!hEvent || !hEventcad || !hEventPreConnect || !hEndSessionEvent || !hMapFile) {
		LogError("Failed to create required synchronization objects.");
		CleanupHandles({ hEvent, hEventcad, hEventPreConnect, hEndSessionEvent, hMapFile });
		return;
	}

	LPVOID data = MapViewOfFile(hMapFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	if (!data) {
		LogError("Failed to map shared memory.");
		CleanupHandles({ hEvent, hEventcad, hEventPreConnect, hEndSessionEvent });
		CloseHandle(hMapFile);
		return;
	}

	int* sessionData = (int*)data;
	DWORD dwSessionId = 0, OlddwSessionId = 0xFFFFFFFF;
	ProcessInfo.hProcess = NULL;
	IsShutdown = false;

	HANDLE events[3] = { hEndSessionEvent, hEventcad, hEventPreConnect };
	HANDLE timeoutEvents[2] = { hEndSessionEvent, hEventcad };

	while (!IsShutdown && serviceStatus.dwCurrentState == SERVICE_RUNNING) {
		DWORD dwEvent;
		if (RDPMODE) {
			dwEvent = WaitForMultipleObjects(3, events, FALSE, 1000);
		}
		else {
			dwEvent = WaitForMultipleObjects(2, timeoutEvents, FALSE, 1000);
		}

		switch (dwEvent) {
		case WAIT_OBJECT_0 + 0: // Shutdown event
			IsShutdown = true;
			break;

		case WAIT_OBJECT_0 + 1: // CAD event
			HandleCADRequest();
			break;

		case WAIT_OBJECT_0 + 2: // Preconnect event
			HandlePreconnect(sessionData, OlddwSessionId);
			break;

		case WAIT_TIMEOUT: // Monitor session or RDPMODE
			if (!MonitorSessions(RDPMODE, dwSessionId, OlddwSessionId)) {
				IsShutdown = true; // Gracefully exit if monitoring fails
			}
			break;

		default: // Error handling for WaitForMultipleObjects
			LogError("Unexpected event error.");
			IsShutdown = true;
			break;
		}
	}

	// terminate process
	if (hEvent)
		SetEvent(hEvent);
	TerminateProcessGracefully(ProcessInfo.hProcess);
	
	// Cleanup resources
	CleanupHandles({ hEvent, hEventcad, hEventPreConnect, hEndSessionEvent });
	if (data) UnmapViewOfFile(data);
	if (hMapFile) CloseHandle(hMapFile);
}
////////////////////////////////////////////////////
// Helper function: Log errors (custom implementation required)
void UltraVNCService::LogError(const char* message) {
	OutputDebugStringA(message); // Replace with appropriate logging mechanism
}
////////////////////////////////////////////////////
// Helper function: Cleanup multiple handles
void UltraVNCService::CleanupHandles(std::initializer_list<HANDLE> handles) {
	for (HANDLE handle : handles) {
		if (handle) CloseHandle(handle);
	}
}
////////////////////////////////////////////////////
// Helper function: Handle Preconnect Event
void UltraVNCService::HandlePreconnect(int* sessionData, DWORD& OlddwSessionId) {
	SetEvent(hEvent); // Notify session event
	DWORD requestedSessionID = *sessionData;

	// Stop existing process
	if (ProcessInfo.hProcess) {
		TerminateProcessGracefully(ProcessInfo.hProcess);
	}

	// Switch to new session
	DWORD dwSessionId = GetActiveSessionId();
	if (dwSessionId != 0xFFFFFFFF) {
		LaunchProcessWin(requestedSessionID, false, true);
		OlddwSessionId = requestedSessionID;
		preconnect_start = true;
	}
}
////////////////////////////////////////////////////
// Helper function: Handle CAD Requests
void UltraVNCService::HandleCADRequest() {
	typedef VOID(WINAPI* SendSas)(BOOL asUser);
	HINSTANCE Inst = LoadLibrary("sas.dll");
	if (Inst) {
		SendSas sendSas = (SendSas)GetProcAddress(Inst, "SendSAS");
		if (sendSas) {
			sendSas(FALSE);
		}
		else {
			LaunchCADProcess();
		}
		FreeLibrary(Inst);
	}
	else {
		LaunchCADProcess();
	}
}
////////////////////////////////////////////////////
// Helper function: Launch CAD Process
void UltraVNCService::LaunchCADProcess() {
	char WORKDIR[MAX_PATH], mycommand[MAX_PATH];
	if (GetModuleFileName(NULL, WORKDIR, MAX_PATH)) {
		char* p = strrchr(WORKDIR, '\\');
		if (p) *p = '\0';
		sprintf_s(mycommand, "%s\\cad.exe", WORKDIR);
		ShellExecute(GetDesktopWindow(), "open", mycommand, "", NULL, SW_SHOWNORMAL);
	}
}
////////////////////////////////////////////////////
bool IsSessionActive(DWORD sessionId) {
	return sessionId != 0xFFFFFFFF;
}
void CloseProcessHandles(PROCESS_INFORMATION& procInfo) {
	if (procInfo.hProcess) CloseHandle(procInfo.hProcess);
	if (procInfo.hThread) CloseHandle(procInfo.hThread);
	procInfo.hProcess = NULL;
	procInfo.hThread = NULL;
}

bool UltraVNCService::MonitorConsoleSession(PROCESS_INFORMATION& procInfo, DWORD& OlddwSessionId, bool& RDPMODE) {
	DWORD dwSessionId = WTSGetActiveConsoleSessionId();
	if (OlddwSessionId != dwSessionId)
		SetEvent(hEvent);
	if (dwSessionId != 0xFFFFFFFF) {
		DWORD dwCode = 0;
		if (ProcessInfo.hProcess == NULL) {
			LaunchProcessWin(dwSessionId, false, false);
			OlddwSessionId = dwSessionId;
			return true;
		}
		else if (GetExitCodeProcess(ProcessInfo.hProcess, &dwCode)) {
			if (dwCode != STILL_ACTIVE) {

				WaitForSingleObject(ProcessInfo.hProcess, 15000);
				CloseProcessHandles(ProcessInfo);

				int sessidcounter = 0;
				while (dwSessionId == 0xFFFFFFFF && !IsShutdown) {
					Sleep(1000);
					dwSessionId = WTSGetActiveConsoleSessionId();
					sessidcounter++;
					if (sessidcounter > 10) break;
				}
				RDPMODE = myIniFile.ReadInt("admin", "rdpmode", 0);
				return true;
			}
		}
		else {
			CloseProcessHandles(ProcessInfo);
			int sessidcounter = 0;

			while (dwSessionId == 0xFFFFFFFF && !IsShutdown) {
				Sleep(1000);
				dwSessionId = WTSGetActiveConsoleSessionId();
				sessidcounter++;
				if (sessidcounter > 10)
					return false;
			}

			RDPMODE = myIniFile.ReadInt("admin", "rdpmode", 0);
			return true;
		}
	}
	return true;
}

bool UltraVNCService::MonitorAndLaunchRdpSession(PROCESS_INFORMATION& procInfo, DWORD& OlddwSessionId, bool& RDPMODE)
{
	if (ProcessInfo.hProcess == NULL) {
		if (IsAnyRDPSessionActive()) {
			LaunchProcessWin(0, true, false);
			OlddwSessionId = 0;
			preconnect_start = false;
			return true;
		}
		else {
			DWORD dwSessionId = 0xFFFFFFFF;
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
			return true;
		}
	}

	if (preconnect_start == true)
		if (!IsSessionStillActive(OlddwSessionId)) SetEvent(hEvent);

	// Monitor process
	DWORD dwCode = 0;
	bool returnvalue = GetExitCodeProcess(ProcessInfo.hProcess, &dwCode);
	if (!returnvalue) {
		//bad handle, thread already terminated
		CloseProcessHandles(ProcessInfo);
		RDPMODE = myIniFile.ReadInt("admin", "rdpmode", 0);
		Sleep(1000);
		return true;
	}

	if (dwCode == STILL_ACTIVE)
		return true;

	if (ProcessInfo.hProcess)
		WaitForSingleObject(ProcessInfo.hProcess, 15000);
	CloseProcessHandles(ProcessInfo);
	RDPMODE = myIniFile.ReadInt("admin", "rdpmode", 0);
	Sleep(1000);
	return true;
}

// Helper function: Monitor Sessions
bool UltraVNCService::MonitorSessions(bool RDPMODE, DWORD& dwSessionId, DWORD& OlddwSessionId) {
	if (RDPMODE && IsAnyRDPSessionActive()) {
		return MonitorAndLaunchRdpSession(ProcessInfo, OlddwSessionId, RDPMODE);
	}
	else {
		return MonitorConsoleSession(ProcessInfo, OlddwSessionId, RDPMODE);
	}

	dwSessionId = GetActiveSessionId();
	if (dwSessionId != OlddwSessionId) {
		SetEvent(hEvent);
		LaunchProcessWin(dwSessionId, false, false);
		OlddwSessionId = dwSessionId;
	}

	return dwSessionId != 0xFFFFFFFF;
}
////////////////////////////////////////////////////
// Helper function: Terminate Process Gracefully
void UltraVNCService::TerminateProcessGracefully(HANDLE hProcess) {
	DWORD dwCode = STILL_ACTIVE;
	while (dwCode == STILL_ACTIVE && !IsShutdown) {
		GetExitCodeProcess(hProcess, &dwCode);
		if (dwCode != STILL_ACTIVE) {
			// Wait for the process to close, with a timeout of 15 seconds
			DWORD waitResult = WaitForSingleObject(ProcessInfo.hProcess, 15000);
			// Clean up process and thread handles
			if (ProcessInfo.hProcess) {
				CloseHandle(ProcessInfo.hProcess);
				ProcessInfo.hProcess = NULL; // Avoid using invalid handle
			}
			if (ProcessInfo.hThread) {
				CloseHandle(ProcessInfo.hThread);
				ProcessInfo.hThread = NULL;
			}
		}
		else {
			// Process is still active; sleep for 1 second
			Sleep(1000);
		}
	}
}
////////////////////////////////////////////////////
// Helper function: Get Active Session ID
DWORD UltraVNCService::GetActiveSessionId() {
	DWORD dwSessionId = WTSGetActiveConsoleSessionId();
	int retryCount = 0;
	while (dwSessionId == 0xFFFFFFFF && retryCount++ < 10) {
		Sleep(1000);
		dwSessionId = WTSGetActiveConsoleSessionId();
	}
	return dwSessionId;
}
////////////////////////////////////////////////////
BOOL UltraVNCService::LaunchProcessWin(DWORD dwSessionId, bool preconnect, bool rdpselect) {
	if (IsShutdown)
		return false;

	BOOL bReturn = FALSE;
	HANDLE hToken = NULL;
	STARTUPINFO StartUPInfo;
	PVOID lpEnvironment = NULL;
	static int counter = 0;

	// Initialize the structures
	ZeroMemory(&StartUPInfo, sizeof(STARTUPINFO));
	ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));

	// Configure the STARTUPINFO structure
	StartUPInfo.wShowWindow = SW_SHOW;
	StartUPInfo.lpDesktop = (LPSTR)"Winsta0\\Default";  // Run the process in the default desktop
	StartUPInfo.cb = sizeof(STARTUPINFO);

	// Set necessary privileges (this function is assumed to be defined elsewhere)
	SetTBCPrivileges();

	// Build the command for the process to be run (this function is assumed to be defined elsewhere)
	createWinvncExeCall(preconnect, rdpselect);

	// Attempt to get the user token for the specified session ID
	if (GetSessionUserTokenWin(&hToken, dwSessionId)) {
		// Create the environment block for the user
		if (CreateEnvironmentBlock(&lpEnvironment, hToken, FALSE)) {
			SetLastError(0);  // Clear any previous error

			// If shutdown is in progress, don't proceed
			if (IsShutdown)
				return false;

			// Attempt to create the process as the user
			if (CreateProcessAsUser(hToken, NULL, app_path_UI, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS, lpEnvironment, NULL, &StartUPInfo, &ProcessInfo)) {
				counter = 0;  // Reset the counter after successful process creation
				bReturn = TRUE;  // Process successfully created
				DWORD error = GetLastError();

#ifdef _DEBUG
				char szText[256];
				sprintf_s(szText, " ++++++ CreateProcessAsUser success %d\n", error);
				OutputDebugString(szText);
#endif
			}
			else {
				DWORD error = GetLastError();
#ifdef _DEBUG
				char szText[256];
				sprintf_s(szText, " ++++++ CreateProcessAsUser failed %d %d %d\n", error, kickrdp, counter);
				OutputDebugString(szText);
#endif

				// Handle the specific error code 233, indicating session issues
				if (error == 233 && kickrdp == 1) {
					counter++;
					if (counter > 3) {
						// Attempt to reconnect to the session and lock the workstation
						typedef BOOLEAN(WINAPI* pWinStationConnect) (HANDLE, ULONG, ULONG, PCWSTR, ULONG);
						HMODULE hlibwinsta = LoadLibrary("winsta.dll");
						pWinStationConnect WinStationConnectF = NULL;

						if (hlibwinsta)
							WinStationConnectF = (pWinStationConnect)GetProcAddress(hlibwinsta, "WinStationConnectW");

						if (WinStationConnectF != NULL) {
							DWORD ID = WTSGetActiveConsoleSessionId();
							WinStationConnectF(0, 0, ID, L"", 0);  // Connect to the active console session
							LockWorkStation();  // Lock the workstation after reconnecting
						}

						Sleep(3000);  // Sleep for 3 seconds before retrying
					}
				}
				else if (error == 233) {
					// If error 233 occurs, retry by creating a remote session process
					CreateRemoteSessionProcess(dwSessionId, true, hToken, NULL, app_path_UI, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS, lpEnvironment, NULL, &StartUPInfo, &ProcessInfo);
					counter = 0;  // Reset counter
					bReturn = TRUE;
				}
			}

			// Clean up the environment block if it was created
			if (lpEnvironment)
				DestroyEnvironmentBlock(lpEnvironment);
		}
		else {
			// If environment block creation fails, attempt to create the process without it
			SetLastError(0);
			if (IsShutdown)
				return false;

			if (CreateProcessAsUser(hToken, NULL, app_path_UI, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo)) {
				counter = 0;  // Reset the counter
				bReturn = TRUE;  // Process successfully created
				DWORD error = GetLastError();

#ifdef _DEBUG
				char szText[256];
				sprintf_s(szText, " ++++++ CreateProcessAsUser without env success %d\n", error);
				OutputDebugString(szText);
#endif
			}
			else {
				DWORD error = GetLastError();
#ifdef _DEBUG
				char szText[256];
				sprintf_s(szText, " ++++++ CreateProcessAsUser without env failed %d\n", error);
				OutputDebugString(szText);
#endif

				// Handle the specific error code 233, indicating session issues
				if (error == 233 && kickrdp == 1) {
					counter++;
					if (counter > 3) {
						// Attempt to reconnect to the session and lock the workstation
						typedef BOOLEAN(WINAPI* pWinStationConnect) (HANDLE, ULONG, ULONG, PCWSTR, ULONG);
						HMODULE hlibwinsta = LoadLibrary("winsta.dll");
						pWinStationConnect WinStationConnectF = NULL;

						if (hlibwinsta)
							WinStationConnectF = (pWinStationConnect)GetProcAddress(hlibwinsta, "WinStationConnectW");

						if (WinStationConnectF != NULL) {
							DWORD ID = WTSGetActiveConsoleSessionId();
							WinStationConnectF(0, 0, ID, L"", 0);  // Connect to the active console session
							LockWorkStation();  // Lock the workstation after reconnecting
						}

						Sleep(3000);  // Sleep for 3 seconds before retrying
					}
				}
				else if (error == 233) {
					// Retry creating a remote session process
					CreateRemoteSessionProcess(dwSessionId, true, hToken, NULL, app_path_UI, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo);
					counter = 0;  // Reset counter
					bReturn = TRUE;
				}
			}
		}

		// Close the user token handle
		CloseHandle(hToken);
	}

	return bReturn;  // Return whether the process was successfully launched
}
////////////////////////////////////////////////////
BOOL UltraVNCService::CreateRemoteSessionProcess(
	IN DWORD dwSessionId,
	IN BOOL bUseDefaultToken,
	IN HANDLE hToken,
	IN LPCWSTR lpApplicationName,
	IN LPSTR A_lpCommandLine,
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

	WCHAR lpCommandLine[255];
	STARTUPINFOW StartupInfo;
	Char2Wchar(lpCommandLine, A_lpCommandLine, 255);  // Convert command line to wide char
	ZeroMemory(&StartupInfo, sizeof(STARTUPINFOW));
	StartupInfo.wShowWindow = SW_SHOW;
	StartupInfo.lpDesktop = (LPWSTR)L"Winsta0\\Winlogon";  // Run process on the Winlogon desktop
	StartupInfo.cb = sizeof(STARTUPINFOW);

	WCHAR szWinStaPath[MAX_PATH];
	BOOL bGetNPName = FALSE;
	WCHAR szNamedPipeName[MAX_PATH] = L"";
	DWORD dwNameLen;
	HINSTANCE hInstWinSta;
	HANDLE hNamedPipe;
	BOOL bRet = FALSE;
	DWORD cbReadBytes, cbWriteBytes;
	DWORD dwEnvLen = 0;
	union { CPAU_PARAM cpauData; BYTE bDump[0x2000]; };
	CPAU_RET_PARAM cpauRetData;
	DWORD dwUsedBytes = sizeof(cpauData);
	LPBYTE pBuffer = (LPBYTE)(&cpauData + 1);

	// Get the path to the winsta.dll library
	GetSystemDirectoryW(szWinStaPath, MAX_PATH);
	lstrcatW(szWinStaPath, L"\\winsta.dll");
	hInstWinSta = LoadLibraryW(szWinStaPath);

	if (hInstWinSta) {
		// Query the terminal server for the named pipe for the session
		pWinStationQueryInformationW pfWinStationQueryInformationW = (pWinStationQueryInformationW)GetProcAddress(hInstWinSta, "WinStationQueryInformationW");
		if (pfWinStationQueryInformationW)
			bGetNPName = pfWinStationQueryInformationW(0, dwSessionId, 0x21, szNamedPipeName, sizeof(szNamedPipeName), &dwNameLen);
		FreeLibrary(hInstWinSta);
	}

	// If unable to get the named pipe, build it manually
	if (!bGetNPName || szNamedPipeName[0] == '\0')
		swprintf(szNamedPipeName, 260, L"\\\\.\\Pipe\\TerminalServer\\SystemExecSrvr\\%d", dwSessionId);

	// Try to open the named pipe
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

	// Initialize the CPAU (Create Process As User) data structure
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

	// Handle environment variables if provided
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

	// Send the CPAU data over the named pipe to the system process
	HANDLE hProcess = NULL;
	if (WriteFile(hNamedPipe, &cpauData, cpauData.cbSize, &cbWriteBytes, NULL)) {
		Sleep(250);  // Give it some time to process

		// Read the response from the named pipe
		if (ReadFile(hNamedPipe, &cpauRetData, sizeof(cpauRetData), &cbReadBytes, NULL)) {
			hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, cpauRetData.ProcInfo.dwProcessId);
#ifdef _DEBUG
			char szText[256];
			sprintf_s(szText, " ++++++cpau %i %p %i %i %p %p\n", cpauRetData.bRetValue, cpauRetData.ProcInfo.hProcess, cpauRetData.ProcInfo.dwProcessId, cpauRetData.ProcInfo.dwThreadId, cpauRetData.ProcInfo.hThread, hProcess);
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

	// If process handle is still 0, try to get it via process ID
	if (lpProcessInformation->hProcess == 0)
		lpProcessInformation->hProcess = hProcess;

	// Handle cases where the process creation failed
	if (lpProcessInformation->hProcess == 0)
		Sleep(5000);  // Sleep and retry

	CloseHandle(hNamedPipe);  // Close the named pipe
	return bRet;
}
////////////////////////////////////////////////////
DWORD UltraVNCService::MarshallString(LPCWSTR pszText, LPVOID, DWORD dwMaxSize, LPBYTE* ppNextBuf, DWORD* pdwUsedBytes)
{
	DWORD dwOffset = *pdwUsedBytes;  // Store the current position of the used bytes in the buffer
	if (!pszText)
		return 0;  // If the string is NULL, return 0 (no data marshaled)

	DWORD dwLen = (DWORD)(wcslen(pszText) + 1) * sizeof(WCHAR);  // Calculate the length of the wide string including null terminator
	if (*pdwUsedBytes + dwLen > dwMaxSize)
		return 0;  // If the buffer would overflow, return 0 (indicating failure)

	// Copy the string into the buffer
	memmove(*ppNextBuf, pszText, dwLen);
	*pdwUsedBytes += dwLen;  // Increment the used bytes counter
	*ppNextBuf += dwLen;  // Move the buffer pointer to the next free space

	return dwOffset;  // Return the starting offset of where the string was marshaled in the buffer
}
////////////////////////////////////////////////////
BOOL UltraVNCService::Char2Wchar(WCHAR* pDest, char* pSrc, int nDestStrLen)
{
	int nSrcStrLen = 0;
	int nOutputBuffLen = 0;
	int retcode = 0;

	// Check for invalid pointers
	if (pDest == NULL || pSrc == NULL)
	{
		return FALSE;  // Return failure if either of the pointers is null
	}

	nSrcStrLen = (int)strlen(pSrc);  // Get the length of the source string
	if (nSrcStrLen == 0)
	{
		return FALSE;  // Return failure if the source string is empty
	}

	nDestStrLen = nSrcStrLen;  // Set the destination length to the source length

	// Check if the destination buffer is large enough to hold the converted string
	if (nDestStrLen > MAXSTRLENGTH - 1)
	{
		return FALSE;  // Return failure if the destination buffer is too small
	}

	memset(pDest, 0, sizeof(TCHAR) * nDestStrLen);  // Clear the destination buffer

	// Perform the conversion from multibyte to wide character using MultiByteToWideChar
	nOutputBuffLen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pSrc, nSrcStrLen, pDest, nDestStrLen);

	// Check if the conversion failed
	if (nOutputBuffLen == 0)
	{
		retcode = GetLastError();  // Get the error code if the conversion failed
		return FALSE;  // Return failure
	}

	pDest[nOutputBuffLen] = '\0';  // Null-terminate the wide string
	return TRUE;  // Return success
}
////////////////////////////////////////////////////
BOOL UltraVNCService::get_winlogon_handle(OUT LPHANDLE  lphUserToken, DWORD mysessionID)
{
	BOOL   bResult = FALSE;
	HANDLE hProcess;
	HANDLE hTokenThis = NULL;
	DWORD ID_session = 0;
	ID_session = mysessionID;   // Session ID to retrieve the user token for
	DWORD Id = 0;

	// Try to find the PID of the winlogon process for the given session
	Id = Find_winlogon(ID_session);  // Custom function to find winlogon by session ID
	if (Id == -1)
		Id = GetwinlogonPid();  // If not found, get the PID of winlogon globally

	// Open the winlogon process using the obtained PID
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Id);
	if (hProcess) {
		// Get the process token from the winlogon process
		OpenProcessToken(hProcess, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, &hTokenThis);

		// Duplicate the token so that it can be used for impersonation or as a primary token
		bResult = DuplicateTokenEx(hTokenThis, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, NULL,
			SecurityImpersonation, TokenPrimary, lphUserToken);

		// Set the session ID in the token information
		SetTokenInformation(*lphUserToken, TokenSessionId, &ID_session, sizeof(DWORD));

		// Close the handles for token and process
		CloseHandle(hTokenThis);
		CloseHandle(hProcess);
	}

	return bResult;  // Return whether the operation was successful
}
////////////////////////////////////////////////////
BOOL UltraVNCService::GetSessionUserTokenWin(OUT LPHANDLE  lphUserToken, DWORD mysessionID)
{
	BOOL   bResult = FALSE;

	// Check if the output token pointer is valid
	if (lphUserToken != NULL) {
		// Call get_winlogon_handle to obtain the user token for the specified session
		bResult = get_winlogon_handle(lphUserToken, mysessionID);
	}

	return bResult;
}
////////////////////////////////////////////////////
DWORD UltraVNCService::GetwinlogonPid()
{
	DWORD dwExplorerLogonPid = 0;    // Initialize PID variable to 0 (default failure value)
	PROCESSENTRY32 procEntry;        // Structure to store process information
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);  // Create a snapshot of all running processes

	// Check if snapshot creation was successful
	if (hSnap == INVALID_HANDLE_VALUE)
		return 0;

	procEntry.dwSize = sizeof(PROCESSENTRY32);   // Set the size of the structure for Process32First
	// Get the first process in the snapshot
	if (!Process32First(hSnap, &procEntry)) {
		CloseHandle(hSnap); // Close handle if we can't get the first process
		return 0;
	}

	// Iterate over all processes in the snapshot
	do {
		// If the process is winlogon.exe, store its PID
		if (_stricmp(procEntry.szExeFile, "winlogon.exe") == 0)
			dwExplorerLogonPid = procEntry.th32ProcessID;

		// Loop until the process list is finished or shutdown flag is set
	} while (!IsShutdown && Process32Next(hSnap, &procEntry));

	CloseHandle(hSnap);  // Close the process snapshot handle
	return dwExplorerLogonPid;  // Return the PID of winlogon.exe or 0 if not found
}
////////////////////////////////////////////////////
DWORD UltraVNCService::Find_winlogon(DWORD SessionId)
{
	PWTS_PROCESS_INFO pProcessInfo = NULL;  // Pointer to store process information
	DWORD ProcessCount = 0;                  // Variable to hold the number of processes
	DWORD Id = -1;                           // Default to -1 if no matching process is found

	// Enumerate processes on the current server (WTS_CURRENT_SERVER_HANDLE)
	if (WTSEnumerateProcessesA(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo, &ProcessCount)) {
		// Iterate through the processes
		for (DWORD CurrentProcess = 0; CurrentProcess < ProcessCount; CurrentProcess++) {
			// Check if the process name is "winlogon.exe" (case insensitive)
			if (_stricmp(pProcessInfo[CurrentProcess].pProcessName, "winlogon.exe") == 0) {
				// Check if the process is associated with the specified SessionId
				if (SessionId == pProcessInfo[CurrentProcess].SessionId) {
					Id = pProcessInfo[CurrentProcess].ProcessId;  // Store the Process ID
					break;  // Exit the loop once the matching process is found
				}
			}
		}
		// Free the memory allocated for process information
		WTSFreeMemory(pProcessInfo);
	}
	return Id;  // Return the process ID or -1 if no match is found
}
////////////////////////////////////////////////////
BOOL UltraVNCService::SetTBCPrivileges(VOID) {
	DWORD dwPID;
	HANDLE hProcess;
	HANDLE hToken;
	LUID Luid;
	TOKEN_PRIVILEGES tpDebug;

	// Step 1: Get the current process ID
	dwPID = GetCurrentProcessId();

	// Step 2: Open the current process with all access rights
	if ((hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)) == NULL)
		return FALSE;  // If the process couldn't be opened, return FALSE

	// Step 3: Open the process token with all access rights
	if (OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken) == 0) {
		CloseHandle(hProcess);  // Close the process handle and return FALSE
		return FALSE;
	}

	// Step 4: Lookup the LUID (Locally Unique Identifier) for the SE_TCB_NAME privilege
	if ((LookupPrivilegeValue(NULL, SE_TCB_NAME, &Luid)) == 0) {
		CloseHandle(hToken);  // Close token handle
		CloseHandle(hProcess);  // Close process handle
		return FALSE;  // If the privilege value couldn't be found, return FALSE
	}

	// Step 5: Set up the TOKEN_PRIVILEGES structure to enable the SE_TCB_NAME privilege
	tpDebug.PrivilegeCount = 1;
	tpDebug.Privileges[0].Luid = Luid;
	tpDebug.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;  // Enable the privilege

	// Step 6: Adjust the token privileges for the current process
	if ((AdjustTokenPrivileges(hToken, FALSE, &tpDebug, sizeof(tpDebug), NULL, NULL)) == 0) {
		CloseHandle(hToken);  // Close token handle
		CloseHandle(hProcess);  // Close process handle
		return FALSE;  // If adjusting the privileges fails, return FALSE
	}

	// Step 7: Check for errors while adjusting the privileges
	if (GetLastError() != ERROR_SUCCESS) {
		CloseHandle(hToken);  // Close token handle
		CloseHandle(hProcess);  // Close process handle
		return FALSE;  // If the error isn't ERROR_SUCCESS, return FALSE
	}

	// Step 8: Close the token and process handles
	CloseHandle(hToken);
	CloseHandle(hProcess);

	// Step 9: Return TRUE to indicate success
	return TRUE;
}
////////////////////////////////////////////////////
void UltraVNCService::wait_for_existing_process()
{
	// Step 1: Continuously loop while the service is not in the shutdown state
	while (!IsShutdown) {
		// Step 2: Try to open an existing event named "Global\\SessionEventUltra" with all access
		hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Global\\SessionEventUltra");

		// Step 3: If the event exists, signal the event (set it)
		if (hEvent != NULL) {
			SetEvent(hEvent);  // This will signal the event, notifying the other process.

			// Step 4: Close the handle to the event since it is no longer needed
			CloseHandle(hEvent);
		}

		// Step 5: Sleep for 1 second before checking again
		Sleep(1000);
	}
}
////////////////////////////////////////////////////
bool UltraVNCService::IsSessionStillActive(int ID)
{
	// Declare pointers and variables
	WTS_SESSION_INFO* pSessions = 0;    // Pointer to store session information
	DWORD nSessions = 0;                 // Number of sessions
	DWORD rdpSessionExists = false;      // Flag to check if session exists

	// Enumerate all sessions on the server
	if (WTSEnumerateSessionsA(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessions, &nSessions)) {

		// Loop through all sessions
		for (DWORD i = 0; i < nSessions && !rdpSessionExists; ++i) {

			// Exclude the "Console" session and check if the session ID matches
			if ((_stricmp(pSessions[i].pWinStationName, "Console") == 0) && (pSessions[i].SessionId == ID))
				rdpSessionExists = true;

			// Check if the session is either active, shadowing, or in the connect query state
			else if ((pSessions[i].SessionId == ID) &&
				(pSessions[i].State == WTSActive ||
					pSessions[i].State == WTSShadow ||
					pSessions[i].State == WTSConnectQuery))
				rdpSessionExists = true;
		}

		// Free memory allocated by WTSEnumerateSessionsA
		WTSFreeMemory(pSessions);
	}

	// Return whether the session exists and is active or in one of the specified states
	return rdpSessionExists ? true : false;
}

bool UltraVNCService::IsServiceInstalled()
{
	bool serviceInstalled = false;
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

	return serviceInstalled;
}

bool UltraVNCService::IsServiceRunning() {
	std::string serviceName = service_name;
	SC_HANDLE serviceManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
	if (!serviceManager) {
		return false;
	}

	DWORD bytesNeeded = 0, servicesReturned = 0, resumeHandle = 0;
	DWORD bufSize = 0;
	EnumServicesStatusEx(
		serviceManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_ACTIVE,
		nullptr, 0, &bytesNeeded, &servicesReturned, &resumeHandle, nullptr);

	bufSize = bytesNeeded;
	BYTE* buffer = new BYTE[bufSize];
	if (!EnumServicesStatusEx(
		serviceManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_ACTIVE,
		buffer, bufSize, &bytesNeeded, &servicesReturned, &resumeHandle, nullptr)) {
		delete[] buffer;
		CloseServiceHandle(serviceManager);
		return false;
	}

	LPENUM_SERVICE_STATUS_PROCESS services = (LPENUM_SERVICE_STATUS_PROCESS)buffer;
	bool isRunning = false;
	for (DWORD i = 0; i < servicesReturned; ++i) {
		if (serviceName == services[i].lpServiceName) {
			isRunning = true;
			break;
		}
	}

	delete[] buffer;
	CloseServiceHandle(serviceManager);
	return isRunning;
}
