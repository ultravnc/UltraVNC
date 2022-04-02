#pragma once
#include "stdhdrs.h"

class Credentials {
private:
	static DWORD GetCurrentUserToken(HANDLE& process, HANDLE& Token);
public:
	static bool RunningAsAdministrator();
};

class DesktopUsersToken {
private:
	HANDLE hProcess, hPToken;
public:
	DesktopUsersToken();
	~DesktopUsersToken();
	HANDLE getDesktopUsersToken();
};