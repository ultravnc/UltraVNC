// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#pragma once
#include "stdhdrs.h"

class Credentials {
private:
	static DWORD GetCurrentUserToken(HANDLE& process, HANDLE& Token, bool RunningFromExternalService);
public:
	static bool RunningAsAdministrator(bool RunningFromExternalService);
};

class DesktopUsersToken {
private:
	HANDLE hProcess, hPToken;
	DesktopUsersToken();
	static DesktopUsersToken* instance;
	DWORD dwExplorerLogonPid = 0;
	char username[257]{};
	HANDLE save_hPtoken = 0;

public:
	static DesktopUsersToken* getInstance();
	~DesktopUsersToken();
	HANDLE getDesktopUsersToken();
	bool GetConsoleUser(char* buffer, UINT size);
};