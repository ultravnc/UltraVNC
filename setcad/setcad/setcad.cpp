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


// setcad.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>

void
Enable_softwareCAD()
{
	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
		{
		return;
		}
	if (RegOpenKeyEx(hkLocal,
		L"System",
		0, KEY_WRITE | KEY_READ,
		&hkLocalKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hkLocal);
		return;
	}
	LONG pref;
	pref=1;
	RegSetValueEx(hkLocalKey, L"SoftwareSASGeneration", 0, REG_DWORD, (LPBYTE) &pref, sizeof(pref));
	RegCloseKey(hkLocalKey);
	RegCloseKey(hkLocal);
}

int _tmain(int argc, _TCHAR* argv[])
{
	Enable_softwareCAD();
	return 0;
}