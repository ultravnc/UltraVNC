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


#include "cadthread.h"
#include "Localization.h"
#include "SettingsManager.h"
#include "credentials.h"

vncCad::vncCad()
{
}

bool vncCad::ISUACENabled()
{
	OSVERSIONINFO OSversion;
	OSversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if (OSversion.dwMajorVersion < 6) return false;
	HKEY hKey;
	if (::RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", &hKey) == ERROR_SUCCESS) {
		DWORD value = 0;
		DWORD tt = 4;
		if (::RegQueryValueExW(hKey, L"EnableLUA", NULL, NULL, (LPBYTE)&value, &tt) == ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return (value != 0);
		}
		RegCloseKey(hKey);
	}
	return false;
}

void vncCad::Enable_softwareCAD()
{
	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
		0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
		return;

	if (RegOpenKeyEx(hkLocal, "System", 0, KEY_WRITE | KEY_READ,
		&hkLocalKey) != ERROR_SUCCESS) {
		RegCloseKey(hkLocal);
		return;
	}
	LONG pref;
	pref = 1;
	RegSetValueEx(hkLocalKey, "SoftwareSASGeneration", 0, REG_DWORD, (LPBYTE)&pref, sizeof(pref));
	RegCloseKey(hkLocalKey);
	RegCloseKey(hkLocal);
}

void vncCad::delete_softwareCAD()
{
	//Beep(1000,10000);
	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
		0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
		return;
	if (RegOpenKeyEx(hkLocal, "System", 0, KEY_WRITE | KEY_READ, &hkLocalKey) != ERROR_SUCCESS) {
		RegCloseKey(hkLocal);
		return;
	}
	RegDeleteValue(hkLocalKey, "SoftwareSASGeneration");
	RegCloseKey(hkLocalKey);
	RegCloseKey(hkLocal);
}

void vncCad::delete_softwareCAD_elevated()
{
	OSVERSIONINFO OSversion;
	OSversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if (OSversion.dwMajorVersion < 6) return;

	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);
	SHELLEXECUTEINFO shExecInfo;
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = "-delsoftwarecad";
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_SHOWNORMAL;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
}

void vncCad::Enable_softwareCAD_elevated()
{
	OSVERSIONINFO OSversion;
	OSversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if (OSversion.dwMajorVersion < 6)
		return;
	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);
	SHELLEXECUTEINFO shExecInfo;
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = "-softwarecad";
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_SHOWNORMAL;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
}

bool vncCad::IsSoftwareCadEnabled()
{
	OSVERSIONINFO OSversion;
	OSversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if (OSversion.dwMajorVersion < 6) return true;

	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
	{
		return 0;
	}
	if (RegOpenKeyEx(hkLocal,
		"System",
		0, KEY_READ,
		&hkLocalKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hkLocal);
		return 0;
	}

	LONG pref = 0;
	ULONG type = REG_DWORD;
	ULONG prefsize = sizeof(pref);

	if (RegQueryValueEx(hkLocalKey,
		"SoftwareSASGeneration",
		NULL,
		&type,
		(LPBYTE)&pref,
		&prefsize) != ERROR_SUCCESS)
	{
		RegCloseKey(hkLocalKey);
		RegCloseKey(hkLocal);
		return false;
	}
	RegCloseKey(hkLocalKey);
	RegCloseKey(hkLocal);
	if (pref != 0) return true;
	else return false;
}

DWORD WINAPI vncCad::Cadthread(LPVOID lpParam)
{
	//SET THREAD in CURRENT INPUTDESKTOP
	HANDLE	hShutdownEventcad;
	hShutdownEventcad = OpenEvent(EVENT_MODIFY_STATE, FALSE, "Global\\SessionEventUltraCad");
	//Switch to InputDesktop
	HDESK desktop = NULL;
	desktop = OpenInputDesktop(0, FALSE,
		DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
		DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
		DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
		DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
	);
	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
	SetThreadDesktop(desktop);

	OSVERSIONINFO OSversion;
	OSversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	//
	if (OSversion.dwMajorVersion >= 6 && settings->RunningFromExternalService() && !IsSoftwareCadEnabled()) {
		DWORD result = MessageBoxSecure(NULL, "UAC is disabled, make registry changes to allow CAD", "Warning", MB_YESNO);
		if (result == IDYES) {
			HANDLE hPToken = DesktopUsersToken::getInstance()->getDesktopUsersToken();
			if (hPToken) {
				char dir[MAX_PATH];
				char exe_file_name[MAX_PATH];
				GetModuleFileName(0, exe_file_name, MAX_PATH);
				strcpy_s(dir, exe_file_name);
				strcat_s(dir, " -softwarecadhelper");

				STARTUPINFO          StartUPInfo{};
				PROCESS_INFORMATION  ProcessInfo{};
				ZeroMemory(&StartUPInfo, sizeof(STARTUPINFO));
				ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));
				StartUPInfo.wShowWindow = SW_SHOW;
				StartUPInfo.lpDesktop = "Winsta0\\Default";
				StartUPInfo.cb = sizeof(STARTUPINFO);
				CreateProcessAsUser(hPToken, NULL, dir, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo);

				DWORD errorcode = GetLastError();
				if (ProcessInfo.hProcess)
					CloseHandle(ProcessInfo.hProcess);
				if (ProcessInfo.hThread)
					CloseHandle(ProcessInfo.hThread);
				if (errorcode == 1314) goto error;
				goto gotome;
			}
		error:
			Enable_softwareCAD_elevated();
		}
	}
gotome:

	//Tell service to call sendSas()
	if (OSversion.dwMajorVersion >= 6) {
		if (hShutdownEventcad == NULL)
			hShutdownEventcad = OpenEvent(EVENT_MODIFY_STATE, FALSE, "Global\\SessionEventUltraCad");
		if (hShutdownEventcad != NULL)
			SetEvent(hShutdownEventcad);
	}
	else { // call cad.exe
		char WORKDIR[MAX_PATH];
		char mycommand[MAX_PATH];
		if (GetModuleFileName(NULL, WORKDIR, MAX_PATH)) {
			char* p = strrchr(WORKDIR, '\\');
			if (p == NULL) return 0;
			*p = '\0';
		}
		strcpy_s(mycommand, "");
		strcat_s(mycommand, WORKDIR);//set the directory
		strcat_s(mycommand, "\\");
		strcat_s(mycommand, "cad.exe");
		ShellExecute(GetDesktopWindow(), "open", mycommand, "", 0, SW_SHOWNORMAL);
	}
	if (hShutdownEventcad)
		CloseHandle(hShutdownEventcad);
	if (old_desktop)
		SetThreadDesktop(old_desktop);
	if (desktop)
		CloseDesktop(desktop);
	return 0;
}