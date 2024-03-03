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
#include "../common/ScopeGuard.h"
#include "SettingsManager.h"
#include "credentials.h"

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
	BOOL   fAdmin{};
	TOKEN_GROUPS* ptg = NULL;
	DWORD  cbTokenGroups{};
	DWORD  dwGroup{};
	PSID   psidAdmin{};
	SetLastError(0);
	HANDLE process = 0;
	HANDLE Token = NULL;

	if (GetCurrentUserToken(process, Token) == 1)
		return true;

	ON_BLOCK_EXIT(CloseHandle, process);
	ON_BLOCK_EXIT(CloseHandle, Token);

	SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

	// Then we must query the size of the group information associated with
	// the token. Note that we expect a FALSE result from GetTokenInformation
	// because we've given it a NULL buffer. On exit cbTokenGroups will tell
	// the size of the group information.

	if (GetTokenInformation(Token, TokenGroups, NULL, 0, &cbTokenGroups))
		return (FALSE);

	// Here we verify that GetTokenInformation failed for lack of a large
	// enough buffer.
	DWORD errorcode = GetLastError();
	if (errorcode != ERROR_INSUFFICIENT_BUFFER)
		return (FALSE);

	// Now we allocate a buffer for the group information.
	// Since _alloca allocates on the stack, we don't have
	// to explicitly deallocate it. That happens automatically
	// when we exit this function.

	if (!(ptg = (_TOKEN_GROUPS*)_malloca(cbTokenGroups)))
		return (FALSE);

	// Now we ask for the group information again.
	// This may fail if an administrator has added this account
	// to an additional group between our first call to
	// GetTokenInformation and this one.

	if (!GetTokenInformation(Token, TokenGroups, ptg, cbTokenGroups,
		&cbTokenGroups)) {
		_freea(ptg);
		return (FALSE);
	}

	// Now we must create a System Identifier for the Admin group.

	if (!AllocateAndInitializeSid(&SystemSidAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psidAdmin)) {
		_freea(ptg);
		return (FALSE);
	}

	// Finally we'll iterate through the list of groups for this access
	// token looking for a match against the SID we created above.

	fAdmin = FALSE;

	for (dwGroup = 0; dwGroup < ptg->GroupCount; dwGroup++) {
		if (EqualSid(ptg->Groups[dwGroup].Sid, psidAdmin)) {
			fAdmin = TRUE;
			break;
		}
	}

	// Before we exit we must explicity deallocate the SID we created.
	_freea(ptg);
	FreeSid(psidAdmin);
	return (FALSE != fAdmin);
}

DesktopUsersToken::DesktopUsersToken()
{
	hProcess = NULL;
	hPToken = NULL;
	DWORD dwExplorerLogonPid = processHelper::GetExplorerLogonPid();
	if (dwExplorerLogonPid != 0) {
		hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, dwExplorerLogonPid);
		OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY
			| TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_SESSIONID
			| TOKEN_READ | TOKEN_WRITE, &hPToken);
	}
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
	return hPToken;
}