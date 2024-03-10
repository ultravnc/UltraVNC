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


// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the AUTH_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// AUTH_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef LDAPAUTH9X_EXPORTS
#define LDAPAUTH9X_API __declspec(dllexport)
#else
#define LDAPAUTH9X_API __declspec(dllimport)
#endif

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <wchar.h>
#include <math.h>
#include <objbase.h>
#include <activeds.h>
#include <Dsgetdc.h>
#include <sddl.h>
#include <assert.h>


#define MAXLEN 256
//#define MAX_PREFERRED_LENGTH    ((DWORD) -1)
//#define NERR_Success            0
//#define LG_INCLUDE_INDIRECT 1
#define BUFSIZE 1024

#define FETCH_NUM 100
void PrintBanner(LPOLESTR pwszBanner);
HRESULT GetObjectGuid(IDirectoryObject * pDO,BSTR &bsGuid);
BOOL RecursiveIsMember(IADsGroup * pADsGroup,LPWSTR pwszMemberGUID,LPWSTR pwszMemberPath, 
                                             BOOL bVerbose, LPOLESTR  pwszUser, LPOLESTR pwszPassword);

HRESULT FindGroup(IDirectorySearch *pSearchBase, //Container to search
                                           LPOLESTR szFindUser, LPOLESTR  pwszUser, LPOLESTR pwszPassword, //Name of user to find.
                                           IADs **ppUser,LPOLESTR szGroup); //Return a pointer to the user



HRESULT FindUserByName(IDirectorySearch *pSearchBase, //Container to search
                                           LPOLESTR szFindUser, LPOLESTR  pwszUser, LPOLESTR pwszPassword, //Name of user to find.
                                           IADs **ppUser); //Return a pointer to the user


BSTR gbsGroup     = NULL; // <Group to check> This is the LDAP path for the group to check
	BSTR gbsMember    = NULL; // <Member to check> 
	BSTR gbsUSER      = NULL;
	BSTR gbsPASS      = NULL; // <Password used for binding to the DC>



LDAPAUTH9X_API BOOL CUGP(char * userin,char *password,char *machine,char *group,int locdom);

