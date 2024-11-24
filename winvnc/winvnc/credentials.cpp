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


#include "stdhdrs.h"
#include "lmcons.h"
#include "vncOSVersion.h"
#include "common/ScopeGuard.h"
#include "SettingsManager.h"
#include "credentials.h"

DesktopUsersToken* DesktopUsersToken::instance = nullptr;

DWORD Credentials::GetCurrentUserToken(HANDLE& process, HANDLE& Token)
{
	if (!settings->RunningFromExternalService()) {
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &Token))
			return 0;
		return 2;
	}

	HWND tray = FindWindow(("Shell_TrayWnd"), 0);
	if (!tray)
		return 0;
	DWORD processId = 0;
	GetWindowThreadProcessId(tray, &processId);
	if (!processId)
		return 0;
	process = OpenProcess(MAXIMUM_ALLOWED, FALSE, processId);
	if (!process)
		return 0;
	OpenProcessToken(process, MAXIMUM_ALLOWED, &Token);
	return 2;
}

bool Credentials::RunningAsAdministrator()
{
	BOOL   fAdmin = FALSE;
	TOKEN_GROUPS* ptg = nullptr;
	DWORD  cbTokenGroups = 0;
	DWORD  dwGroup = 0;
	PSID   psidAdmin = nullptr;
	SetLastError(0);
	HANDLE process = nullptr;
	HANDLE Token = nullptr;

	if (!GetCurrentUserToken(process, Token) == 1)
		return false;

	ON_BLOCK_EXIT(CloseHandle, process);
	ON_BLOCK_EXIT(CloseHandle, Token);

	SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

	// Get size of the group information
	if (!GetTokenInformation(Token, TokenGroups, nullptr, 0, &cbTokenGroups)) {
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			return false;
		}
	}

	// Allocate memory for group information
	ptg = (TOKEN_GROUPS*)_malloca(cbTokenGroups);
	if (!ptg) {
		return false;
	}

	// Retrieve group information
	if (!GetTokenInformation(Token, TokenGroups, ptg, cbTokenGroups, &cbTokenGroups)) {
		return false;
	}

	// Create SID for Administrators group
	if (!AllocateAndInitializeSid(
		&SystemSidAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&psidAdmin)) {
		return false;
	}

	// Check group membership
	for (dwGroup = 0; dwGroup < ptg->GroupCount; dwGroup++) {
		if (EqualSid(ptg->Groups[dwGroup].Sid, psidAdmin)) {
			fAdmin = TRUE;
			break;
		}
	}

	/// Cleanup SID
	if (psidAdmin) {
		FreeSid(psidAdmin);
	}

	return fAdmin;
}

DesktopUsersToken* DesktopUsersToken::getInstance() {
	if (instance == nullptr) {
		instance = new DesktopUsersToken();
	}
	return instance;
}

DesktopUsersToken::DesktopUsersToken()
{
	hProcess = NULL;
	hPToken = NULL;
}

DesktopUsersToken::~DesktopUsersToken()
{
	if (hProcess)
		CloseHandle(hProcess);
	if (hPToken)
		CloseHandle(hPToken);
}

HANDLE DesktopUsersToken::getDesktopUsersToken()
{
	DWORD explorerLogonPid = processHelper::GetExplorerLogonPid();
	if (explorerLogonPid != 0) {
		vnclog.Print(LL_LOGSCREEN, "explorer shell found");
	}
	else
		vnclog.Print(LL_LOGSCREEN, "explorer shell NOT found");

	
	if (explorerLogonPid != 0 && dwExplorerLogonPid != explorerLogonPid) {
		vnclog.Print(LL_INTWARN, VNCLOG("DesktopUsersToken failed OpenProcess error %i\n"), GetLastError());

		vnclog.Print(LL_INTWARN, VNCLOG("GetExplorerLogonPid %i\n"), explorerLogonPid);
		hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, explorerLogonPid);
		if (hProcess == NULL) {
			vnclog.Print(LL_LOGSCREEN, "UsersToken Failed");
			vnclog.Print(LL_INTWARN, VNCLOG("DesktopUsersToken failed OpenProcess error %i\n"), GetLastError());
			return NULL;
		}
		vnclog.Print(LL_LOGSCREEN, "UsersToken found");
		if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY
			| TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_SESSIONID
			| TOKEN_READ | TOKEN_WRITE, &hPToken)) {
			vnclog.Print(LL_INTWARN, VNCLOG("OpenProcessToken failed  %i\n"), GetLastError());
			vnclog.Print(LL_LOGSCREEN, "OpenProcessToken Failed");
			return NULL;
		}
		vnclog.Print(LL_LOGSCREEN, "OpenProcessToken OK");
	}


	dwExplorerLogonPid = explorerLogonPid;
	return hPToken;
}

bool DesktopUsersToken::GetConsoleUser(char* buffer, UINT size)
{
	HANDLE hPToken = DesktopUsersToken::getInstance()->getDesktopUsersToken();
	if (hPToken == NULL) {
		strcpy_s(buffer, UNLEN + 1, "");
		return 0;
	}

	if (hPToken == save_hPtoken) {
		strcpy_s(buffer, UNLEN + 1, username);
		return strlen(username) != 0;
	}

	save_hPtoken = hPToken;
	char aa[16384]{};
	// token user
	TOKEN_USER* ptu;
	DWORD needed;
	ptu = (TOKEN_USER*)aa;
	if (GetTokenInformation(hPToken, TokenUser, ptu, 16384, &needed))
	{
		char  DomainName[64];
		memset(DomainName, 0, sizeof(DomainName));
		DWORD DomainSize;
		DomainSize = sizeof(DomainName) - 1;
		SID_NAME_USE SidType;
		DWORD dwsize = size;
		LookupAccountSid(NULL, ptu->User.Sid, buffer, &dwsize, DomainName, &DomainSize, &SidType);
		strcpy_s(username, UNLEN + 1, buffer);
		return 1;
	}
	strcpy_s(buffer, UNLEN + 1, "");
	strcpy_s(username, UNLEN + 1, "");
	return 0;
}