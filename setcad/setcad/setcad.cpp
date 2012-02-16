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
	return 0;
}