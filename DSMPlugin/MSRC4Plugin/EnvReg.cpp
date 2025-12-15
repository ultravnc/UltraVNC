// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2005 Sean E. Covel All Rights Reserved.
//


#include "EnvReg.h"

// Ugly function to pull environment variables out of the registry.
// SERVICES can't access user or system environment variables directly. (?)
BOOL GetEnvironmentVariableFromRegistry(LPTSTR lpName, LPTSTR buffer, DWORD nSize)
{
	// Specific to %ProgramFiles%
	TCHAR * PROGRAM_FILES = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion");

	// System environment
	TCHAR * SYSTEM_ENV = _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment");
	// User environment
	TCHAR * USER_ENV = _T("Environment");

	// Registry handler class
	REGISTRY *regClass;

	buffer[0] = '\0';

	// %ProgramFiles% variable (Special case)
	if (_tcsicmp(lpName,PROGRAMFILES) == 0)
	{

		regClass = new REGISTRY( HKEY_LOCAL_MACHINE, PROGRAM_FILES, false);
		regClass->ReadItem(buffer, nSize, PROGRAMFILESDIR, NULL);
		delete regClass;

		if (strlen(buffer) == 0)
			return false;

	}
	else
	{

		// Try the USER env first
		regClass = new REGISTRY( HKEY_CURRENT_USER, USER_ENV, false);
		regClass->ReadItem(buffer, nSize, lpName, NULL);
		delete regClass;

		if (strlen(buffer)== 0)
		{

			// Try the SYSTEM env
			buffer[0] = '\0';

			regClass = new REGISTRY( HKEY_LOCAL_MACHINE, SYSTEM_ENV, false);
			regClass->ReadItem(buffer, nSize, lpName, NULL);
			delete regClass;

			if (strlen(buffer)== 0)
				return false;
		}

	}

	return true;
}


