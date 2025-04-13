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
#include "omnithread.h"
#include <objbase.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "SettingsManager.h"
#include "winvnc.h"

// Marscha@2004 - authSSP: from stdhdrs.h, required for logging
#include "vnclog.h"
extern VNCLog vnclog;
#include "common/inifile.h"

// Marscha@2004 - authSSP: end of change

#include "Localization.h" // Act : add localization on messages

typedef BOOL(*CheckUserGroupPasswordFn)(char* userin, char* password, char* machine, char* group, int locdom);

int CheckUserGroupPasswordUni(char* userin, char* password, const char* machine);
int CheckUserGroupPasswordUni2(char* userin, char* password, const char* machine);

// Marscha@2004 - authSSP: if "New MS-Logon" is checked, call CheckUserPasswordSDUni
BOOL IsNewMSLogon();
//char *AddToModuleDir(char *filename, int length);
typedef int (*CheckUserPasswordSDUniFn)(const char* userin, const char* password, const char* machine, TCHAR* szMslogonLog);
CheckUserPasswordSDUniFn CheckUserPasswordSDUni = 0;

#define MAXSTRING 254

///////////////////////////////////////////////////////////
bool CheckAD()
{
	HMODULE hModule = LoadLibrary("Activeds.dll");
	if (hModule)
	{
		FreeLibrary(hModule);
		return true;
	}
	return false;
}

bool CheckNetapi95()
{
	HMODULE hModule = LoadLibrary("netapi32.dll");
	if (hModule)
	{
		FreeLibrary(hModule);
		return true;
	}
	return false;
}

bool CheckDsGetDcNameW()
{
	HMODULE hModule = LoadLibrary("netapi32.dll");
	if (hModule)
	{
		FARPROC test = NULL;
		test = GetProcAddress(hModule, "DsGetDcNameW");
		FreeLibrary(hModule);
		if (test) return true;
	}
	return false;
}

bool CheckNetApiNT()
{
	HMODULE hModule = LoadLibrary("radmin32.dll");
	if (hModule)
	{
		FreeLibrary(hModule);
		return true;
	}
	return false;
}

int CheckUserGroupPasswordUni(char* userin, char* password, const char* machine)
{
	int result = 0;
	HMODULE hModuleAuthSSP = NULL;
	// Marscha@2004 - authSSP: if "New MS-Logon" is checked, call CUPSDV2 in authSSP.dll,
	// else call "old" MS-Logon method.
	if (IsNewMSLogon()) {
		char szCurrentDir[MAX_PATH];
		strcpy_s(szCurrentDir, winvncFolder);
		strcat_s(szCurrentDir, "\\authSSPV2.dll");
		
		hModuleAuthSSP = LoadLibrary(szCurrentDir);
		if (hModuleAuthSSP) {
			CheckUserPasswordSDUni = (CheckUserPasswordSDUniFn)GetProcAddress(hModuleAuthSSP, "CUPSDV2");
			vnclog.Print(LL_INTINFO, VNCLOG("GetProcAddress"));
			CoInitialize(NULL);
			result = CheckUserPasswordSDUni(userin, password, machine, settings->getLogFile());
			vnclog.Print(LL_INTINFO, "CheckUserPasswordSDUni result=%i", result);
			CoUninitialize();
			FreeLibrary(hModuleAuthSSP);
			//result = CheckUserPasswordSDUni(userin, password, machine);
		}
		else {
			LPCTSTR sz_ID_AUTHSSP_NOT_FO = // to be moved to localization.h
				"You selected MS-Logon, but authSSPV2.dll\nwas not found.Check you installation";
			MessageBoxSecure(NULL, sz_ID_AUTHSSP_NOT_FO, sz_ID_WARNING, MB_OK);
		}
	}
	else
		result = CheckUserGroupPasswordUni2(userin, password, machine);
	return result;
}

int CheckUserGroupPasswordUni2(char* userin, char* password, const char* machine)
{
	HMODULE hModuleAdmin = NULL;
	HMODULE hModuleWorkgroup = NULL;
	HMODULE hModuleLdapAuth = NULL;
	HMODULE hModuleLdapAuthNT4 = NULL;
	HMODULE hModuleLdapAuth9x = NULL;

	CheckUserGroupPasswordFn CheckUserGroupPasswordAdmin = NULL;
	CheckUserGroupPasswordFn CheckUserGroupPasswordWorkgroup = NULL;
	CheckUserGroupPasswordFn CheckUserGroupPasswordLdapAuth = NULL;
	CheckUserGroupPasswordFn CheckUserGroupPasswordLdapAuthNT4 = NULL;
	CheckUserGroupPasswordFn CheckUserGroupPasswordLdapAuth9x = NULL;

	int result = 0;
	BOOL NT4OS = false;
	BOOL W2KOS = false;
	char clientname[256];
	strcpy_s(clientname, 256, machine);
	if (!CheckNetapi95() && !CheckNetApiNT())
	{
		return false;
	}
	OSVERSIONINFO VerInfo{};
	VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&VerInfo))   // If this fails, something has gone wrong
	{
		return FALSE;
	}
	if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT && VerInfo.dwMajorVersion == 4)
	{
		NT4OS = true;
	}
	if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT && VerInfo.dwMajorVersion >= 5)
	{
		W2KOS = true;
	}

	char szCurrentDir[MAX_PATH];
	strcpy_s(szCurrentDir, winvncFolder);
	strcat_s(szCurrentDir, "\\authadmin.dll");
	
	hModuleAdmin = LoadLibrary(szCurrentDir);
	if (hModuleAdmin)
	{
		CheckUserGroupPasswordAdmin = (CheckUserGroupPasswordFn)GetProcAddress(hModuleAdmin, "CUGP");
	}

	strcpy_s(szCurrentDir, winvncFolder);
	strcat_s(szCurrentDir, "\\workgrpdomnt4.dll");
	
	hModuleWorkgroup = LoadLibrary(szCurrentDir);
	if (hModuleWorkgroup)
	{
		CheckUserGroupPasswordWorkgroup = (CheckUserGroupPasswordFn)GetProcAddress(hModuleWorkgroup, "CUGP");
	}
	strcpy_s(szCurrentDir, winvncFolder);
	strcat_s(szCurrentDir, "\\ldapauth.dll");
	
	hModuleLdapAuth = LoadLibrary(szCurrentDir);
	if (hModuleLdapAuth)
	{
		CheckUserGroupPasswordLdapAuth = (CheckUserGroupPasswordFn)GetProcAddress(hModuleLdapAuth, "CUGP");
	}
	strcpy_s(szCurrentDir, winvncFolder);
	strcat_s(szCurrentDir, "\\ldapauthnt4.dll");

	hModuleLdapAuthNT4 = LoadLibrary(szCurrentDir);
	if (hModuleLdapAuthNT4)
	{
		CheckUserGroupPasswordLdapAuthNT4 = (CheckUserGroupPasswordFn)GetProcAddress(hModuleLdapAuthNT4, "CUGP");
	}
	strcpy_s(szCurrentDir, winvncFolder);
	strcat_s(szCurrentDir, "\\ldapauth9x.dll");

	hModuleLdapAuth9x = LoadLibrary(szCurrentDir);
	if (hModuleLdapAuth9x)
	{
		CheckUserGroupPasswordLdapAuth9x = (CheckUserGroupPasswordFn)GetProcAddress(hModuleLdapAuth9x, "CUGP");
	}
	//////////////////////////////////////////////////
	// Load reg settings
	//////////////////////////////////////////////////

	//////////////////////////////////////////////////
	// logon user only works on Windows NT>
	// Windows NT4/Windows 2000 only as service (system account)
	// Windows XP> works also as application
	// Group is not used...admin access rights is needed
	// MS keep changes there security model for each version....
	//////////////////////////////////////////////////
////////////////////////////////////////////////////
	if (strcmp(settings->getGroup1(), "") == 0 && strcmp(settings->getGroup2(), "") == 0 && strcmp(settings->getGroup3(), "") == 0)
		if (NT4OS || W2KOS) {
			if (CheckUserGroupPasswordAdmin)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordAdmin(userin, password, clientname, settings->getGroup1(), settings->getLocdom1());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "authadmin.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
	if (result == 1) goto accessOK;

	if (strcmp(settings->getGroup1(), "") != 0)
	{
		///////////////////////////////////////////////////
		// NT4 domain and workgroups
		//
		///////////////////////////////////////////////////
		{
			
			if (CheckUserGroupPasswordWorkgroup)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordWorkgroup(userin, password, clientname, settings->getGroup1(), settings->getLocdom1());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "workgrpdomnt4.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
		if (result == 1) goto accessOK;
		/////////////////////////////////////////////////////////////////
		if (CheckAD() && W2KOS && (settings->getLocdom1() == 2 || settings->getLocdom1() == 3))
		{
			if (CheckUserGroupPasswordLdapAuth)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordLdapAuth(userin, password, clientname, settings->getGroup1(), settings->getLocdom1());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "ldapauth.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
		if (result == 1) goto accessOK;
		//////////////////////////////////////////////////////////////////////
		if (CheckAD() && NT4OS && CheckDsGetDcNameW() && (settings->getLocdom1() == 2 || settings->getLocdom1() == 3))
		{
			
			if (CheckUserGroupPasswordLdapAuthNT4)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordLdapAuthNT4(userin, password, clientname, settings->getGroup1(), settings->getLocdom1());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "ldapauthnt4.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
		if (result == 1) goto accessOK;
		//////////////////////////////////////////////////////////////////////
		if (CheckAD() && !NT4OS && !W2KOS && (settings->getLocdom1() == 2 || settings->getLocdom1() == 3))
		{
			if (CheckUserGroupPasswordLdapAuth9x)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordLdapAuth9x(userin, password, clientname, settings->getGroup1(), settings->getLocdom1());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "ldapauth9x.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
		if (result == 1) goto accessOK;
	}
	/////////////////////////////////////////////////
	if (strcmp(settings->getGroup2(), "") != 0)
	{
		///////////////////////////////////////////////////
		// NT4 domain and workgroups
		//
		///////////////////////////////////////////////////
		{
			if (CheckUserGroupPasswordWorkgroup)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordWorkgroup(userin, password, clientname, settings->getGroup2(), settings->getLocdom2());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "workgrpdomnt4.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
		if (result == 1) goto accessOK;
		//////////////////////////////////////////////////////
		if (NT4OS || W2KOS) {
			if (CheckUserGroupPasswordAdmin)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordAdmin(userin, password, clientname, settings->getGroup2(), settings->getLocdom2());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "authadmin.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
		if (result == 1) goto accessOK;
		//////////////////////////////////////////////////////////////////
		if (CheckAD() && W2KOS && (settings->getLocdom2() == 2 || settings->getLocdom2() == 3))
		{
			if (CheckUserGroupPasswordLdapAuth)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordLdapAuth(userin, password, clientname, settings->getGroup2(), settings->getLocdom2());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "ldapauth.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
		if (result == 1) goto accessOK;
		///////////////////////////////////////////////////////////////////////
		if (CheckAD() && NT4OS && CheckDsGetDcNameW() && (settings->getLocdom2() == 2 || settings->getLocdom2() == 3))
		{
			if (CheckUserGroupPasswordLdapAuthNT4)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordLdapAuthNT4(userin, password, clientname, settings->getGroup2(), settings->getLocdom2());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "ldapauthnt4.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
		if (result == 1) goto accessOK;
		///////////////////////////////////////////////////////////////////////
		if (CheckAD() && !NT4OS && !W2KOS && (settings->getLocdom2() == 2 || settings->getLocdom2() == 3))
		{
			if (CheckUserGroupPasswordLdapAuth9x)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordLdapAuth9x(userin, password, clientname, settings->getGroup2(), settings->getLocdom2());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "ldapauth9x.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
		if (result == 1) goto accessOK;
	}
	////////////////////////////
	if (strcmp(settings->getGroup3(), "") != 0)
	{
		///////////////////////////////////////////////////
		// NT4 domain and workgroups
		//
		///////////////////////////////////////////////////
		{
			if (CheckUserGroupPasswordWorkgroup)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordWorkgroup(userin, password, clientname, settings->getGroup3(), settings->getLocdom3());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "workgrpdomnt4.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
		if (result == 1) goto accessOK2;
		////////////////////////////////////////////////////////
		if (NT4OS || W2KOS) {
			if (CheckUserGroupPasswordAdmin)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordAdmin(userin, password, clientname, settings->getGroup3(), settings->getLocdom3());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "authadmin.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
		if (result == 1) goto accessOK2;
		////////////////////////////////////////////////////////////////
		if (CheckAD() && W2KOS && (settings->getLocdom3() == 2 || settings->getLocdom3() == 3))
		{
			if (CheckUserGroupPasswordLdapAuth)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordLdapAuth(userin, password, clientname, settings->getGroup3(), settings->getLocdom3());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "ldapauth.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
		if (result == 1) goto accessOK2;
		///////////////////////////////////////////////////////////////////
		if (CheckAD() && NT4OS && CheckDsGetDcNameW() && (settings->getLocdom3() == 2 || settings->getLocdom3() == 3))
		{
			if (CheckUserGroupPasswordLdapAuthNT4)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordLdapAuthNT4(userin, password, clientname, settings->getGroup3(), settings->getLocdom3());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "ldapauthnt4.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
		if (result == 1) goto accessOK2;
		///////////////////////////////////////////////////////////////////
		if (CheckAD() && !NT4OS && !W2KOS && (settings->getLocdom3() == 2 || settings->getLocdom3() == 3))
		{
			if (CheckUserGroupPasswordLdapAuth9x)
			{
				CoInitialize(NULL);
				result = CheckUserGroupPasswordLdapAuth9x(userin, password, clientname, settings->getGroup3(), settings->getLocdom3());
				CoUninitialize();
			}
			else
			{
				MessageBoxSecure(NULL, "ldapauth9x.dll not found", sz_ID_WARNING, MB_OK);
				result = 0;
			}
		}
		if (result == 1) goto accessOK2;
	}

	/////////////////////////////////////////////////
	// If we reach this place auth failed
	/////////////////////////////////////////////////
	{
		typedef BOOL(*LogeventFn)(char* machine, char* user, char* szMslogonLog);
		LogeventFn Logevent = 0;
		char szCurrentDir[MAX_PATH];
		strcpy_s(szCurrentDir, winvncFolder);
		strcat_s(szCurrentDir, "\\logging.dll");
		
		HMODULE hModule = LoadLibrary(szCurrentDir);
		if (hModule)
		{
			Logevent = (LogeventFn)GetProcAddress(hModule, "LOGFAILEDUSERV2");
			Logevent((char*)clientname, userin, settings->getLogFile());
			FreeLibrary(hModule);
		}
		if (hModuleAdmin) FreeLibrary(hModuleAdmin);
		if (hModuleWorkgroup) FreeLibrary(hModuleWorkgroup);
		if (hModuleLdapAuth) FreeLibrary(hModuleLdapAuth);
		if (hModuleLdapAuthNT4) FreeLibrary(hModuleLdapAuthNT4);
		if (hModuleLdapAuth9x) FreeLibrary(hModuleLdapAuth9x);
		hModuleAdmin = NULL;
		hModuleWorkgroup = NULL;
		hModuleLdapAuth = NULL;
		hModuleLdapAuthNT4 = NULL;
		hModuleLdapAuth9x = NULL;
		return result;
	}

accessOK://full access
	{
		typedef BOOL(*LogeventFn)(char* machine, char* user, char* szMslogonLog);
		LogeventFn Logevent = 0;
		char szCurrentDir[MAX_PATH];
		strcpy_s(szCurrentDir, winvncFolder);
		strcat_s(szCurrentDir, "\\logging.dll");
		
		HMODULE hModule = LoadLibrary(szCurrentDir);
		if (hModule)
		{
			Logevent = (LogeventFn)GetProcAddress(hModule, "LOGLOGONUSERV2");
			Logevent((char*)clientname, userin, settings->getLogFile());
			FreeLibrary(hModule);
		}


		if (hModuleAdmin) FreeLibrary(hModuleAdmin);
		if (hModuleWorkgroup) FreeLibrary(hModuleWorkgroup);
		if (hModuleLdapAuth) FreeLibrary(hModuleLdapAuth);
		if (hModuleLdapAuthNT4) FreeLibrary(hModuleLdapAuthNT4);
		if (hModuleLdapAuth9x) FreeLibrary(hModuleLdapAuth9x);
		hModuleAdmin = NULL;
		hModuleWorkgroup = NULL;
		hModuleLdapAuth = NULL;
		hModuleLdapAuthNT4 = NULL;
		hModuleLdapAuth9x = NULL;

		return result;
	}

accessOK2://readonly
	{
		typedef BOOL(*LogeventFn)(char* machine, char* user, char* szMslogonLog);
		LogeventFn Logevent = 0;
		char szCurrentDir[MAX_PATH];
		strcpy_s(szCurrentDir, winvncFolder);
		strcat_s(szCurrentDir, "\\logging.dll");

		HMODULE hModule = LoadLibrary(szCurrentDir);
		if (hModule)
		{
			Logevent = (LogeventFn)GetProcAddress(hModule, "LOGLOGONUSERV2");
			Logevent((char*)clientname, userin, settings->getLogFile());
			FreeLibrary(hModule);
		}
		if (hModuleAdmin) FreeLibrary(hModuleAdmin);
		if (hModuleWorkgroup) FreeLibrary(hModuleWorkgroup);
		if (hModuleLdapAuth) FreeLibrary(hModuleLdapAuth);
		if (hModuleLdapAuthNT4) FreeLibrary(hModuleLdapAuthNT4);
		if (hModuleLdapAuth9x) FreeLibrary(hModuleLdapAuth9x);
		hModuleAdmin = NULL;
		hModuleWorkgroup = NULL;
		hModuleLdapAuth = NULL;
		hModuleLdapAuthNT4 = NULL;
		hModuleLdapAuth9x = NULL;
		result = 2;
	}
	if (hModuleAdmin) FreeLibrary(hModuleAdmin);
	if (hModuleWorkgroup) FreeLibrary(hModuleWorkgroup);
	if (hModuleLdapAuth) FreeLibrary(hModuleLdapAuth);
	if (hModuleLdapAuthNT4) FreeLibrary(hModuleLdapAuthNT4);
	if (hModuleLdapAuth9x) FreeLibrary(hModuleLdapAuth9x);
	hModuleAdmin = NULL;
	hModuleWorkgroup = NULL;
	hModuleLdapAuth = NULL;
	hModuleLdapAuthNT4 = NULL;
	hModuleLdapAuth9x = NULL;
	return result;
}

// Marscha@2004 - authSSP: Is New MS-Logon activated?
BOOL IsNewMSLogon() {
	return settings->getNewMSLogon();
}