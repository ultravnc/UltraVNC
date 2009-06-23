//  Copyright (C) 2005 Sean E. Covel All Rights Reserved.
//
//  Created by Sean E. Covel based on UltraVNC's excellent TestPlugin project.
//
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
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://home.comcast.net/~msrc4plugin
// or
// mail: msrc4plugin@comcast.net
//
//
//

#include "EnvReg.h"

//Ugly function to pull environment variables out of the registry.  
//SERVICES can't access user or system environment variables directly. (?)
BOOL GetEnvironmentVariableFromRegistry(LPTSTR lpName, LPTSTR buffer, DWORD nSize)
{
	//specific to %ProgramFiles%
	TCHAR * PROGRAM_FILES = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion");

	// System environment
	TCHAR * SYSTEM_ENV = _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment");
	// User environment
	TCHAR * USER_ENV = _T("Environment");

	//registry handler class
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

		//Try the USER env first
		regClass = new REGISTRY( HKEY_CURRENT_USER, USER_ENV, false);
		regClass->ReadItem(buffer, nSize, lpName, NULL);
		delete regClass;

		if (strlen(buffer)== 0)
		{

			//Try the SYSTEM env
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


