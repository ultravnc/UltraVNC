//  Copyright (C) 2005 Sean E. Covel All Rights Reserved.
//
//  Created by Sean E. Covel
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
/////////////////////////////////////////////////////////////////////////////

#include "utils.h"

//Ugly function to pull environment variables
//wrapper to decide if getenv or registry should be used.
BOOL GetEnvVar(LPTSTR lpName, LPTSTR buffer, DWORD nSize)
{
	BOOL bRC = false;
	
	char * pEnvVar = 0;


	buffer[0] = '\0';

	//try the registry first
	bRC = GetEnvironmentVariableFromRegistry(lpName, buffer, nSize);

	if (strlen(buffer) == 0)
	{
		//Failed, try GetEnvironmentVariable

		buffer[0] = '\0';
		pEnvVar = getenv(lpName);

		if (pEnvVar != 0)
			strncpy(buffer, pEnvVar, nSize);


		//if (pEnvVar != 0)
		//	strncpy_s(buffer,nSize, pEnvVar, nSize);

		//GetEnvironmentVariable() was returning strings with additional variables in the string.
		//      ex) programfiles = "%systemdisk%\program files" (or some such thing...)
		//getenv() seems to have all the variables already resolved.
		//      ex) programfiles = "c:\program files"

		//if (GetEnvironmentVariable(lpName, buffer, nSize) > nSize)
		//	PrintLog((DEST,"GetEnvironmentVariable failed - buffer too small"));

		if (strlen(buffer) ==0)
			bRC = false;
		else
			bRC = true;
#ifdef _WITH_LOG  
		PrintLog((DEST,"GetEnvVar (getenv) %s = %s ",lpName, buffer));
#endif 
	}
	else
	{
#ifdef _WITH_LOG  
		PrintLog((DEST,"GetEnvVar (registry) %s = %s ",lpName, buffer));
#endif  
	}

	return bRC;
}



int WhatWindowsVer(void)
{

//Modified From an MSDN example...

OSVERSIONINFOEX osvi;
BOOL bOsVersionInfoEx;
int iVersion;

iVersion = 0;
// Try calling GetVersionEx using the OSVERSIONINFOEX structure.
// If that fails, try using the OSVERSIONINFO structure.

ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi);

//success is non-zero return, if not success...
if( !(bOsVersionInfoEx))
{
  osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
  if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) ) 
     return FALSE;
}

switch (osvi.dwPlatformId)
{
  // Test for the Windows NT product family.
  case VER_PLATFORM_WIN32_NT:

     // Test for the specific product family.
     if ( osvi.dwMajorVersion >= 6 )	//Vista or Win2008 or Win7 or above //pgm added
        iVersion = WINVISTA; //pgm added

     if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )	//Win2003
        iVersion = WIN2003;

     if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )	//WinXP
        iVersion = WINXP;

     if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )	//Win2000
        iVersion = WIN2000;

     if ( osvi.dwMajorVersion <= 4 )	//WinNT
        iVersion = WINNT;

     break;

  // Test for the Windows 95 product family.
  case VER_PLATFORM_WIN32_WINDOWS:

     if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
     {
         if ( osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B' )
            iVersion = WIN95B;	//Win95 osr2
		 else
			iVersion = WIN95;	//Win95
     } 

     if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
     {
         if ( osvi.szCSDVersion[1] == 'A' )
			 iVersion = WIN98SE;	//Win98SE
		 else
		 iVersion = WIN98;		//Win98
     } 

     if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
     {
		  iVersion = WINME;	//WinME
     } 
     break;

  case VER_PLATFORM_WIN32s:

		 iVersion = WIN3_1;	//Win3.1
     break;
}

return iVersion; 


}



#define BUFSIZE 80

int WinVer()
{
   OSVERSIONINFOEX osvi;
   BOOL bOsVersionInfoEx;

   // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
   // If that fails, try using the OSVERSIONINFO structure.

   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

   bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi);

   if( !(bOsVersionInfoEx) )
   {
      osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
      if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) ) 
         return FALSE;
   }

   switch (osvi.dwPlatformId)
   {
      // Test for the Windows NT product family.
      case VER_PLATFORM_WIN32_NT:

#ifdef _WITH_LOG  
         // Test for the specific product family.
         if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
            PrintLog((DEST,"Microsoft Windows Server 2003 family, "));

         if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
            PrintLog((DEST,"Microsoft Windows XP "));

         if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
            PrintLog((DEST,"Microsoft Windows 2000 "));

         if ( osvi.dwMajorVersion <= 4 )
            PrintLog((DEST,"Microsoft Windows NT "));
#endif  
         // Test for specific product on Windows NT 4.0 SP6 and later.
         if( bOsVersionInfoEx )
         {
            // Test for the workstation type.
         }
         else  // Test for specific product on Windows NT 4.0 SP5 and earlier
         {
            HKEY hKey;
            char szProductType[BUFSIZE];
            DWORD dwBufLen=BUFSIZE;
            LONG lRet;

            lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
               "SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
               0, KEY_QUERY_VALUE, &hKey );
            if( lRet != ERROR_SUCCESS )
               return FALSE;

            lRet = RegQueryValueEx( hKey, "ProductType", NULL, NULL,
               (LPBYTE) szProductType, &dwBufLen);
            if( (lRet != ERROR_SUCCESS) || (dwBufLen > BUFSIZE) )
               return FALSE;

            RegCloseKey( hKey );

#ifdef _WITH_LOG  
            if ( lstrcmpi( "WINNT", szProductType) == 0 )
               PrintLog((DEST,"Workstation " ));
            if ( lstrcmpi( "LANMANNT", szProductType) == 0 )
               PrintLog((DEST,"Server " ));
            if ( lstrcmpi( "SERVERNT", szProductType) == 0 )
               PrintLog((DEST,"Advanced Server " ));

            PrintLog((DEST,"%d.%d ", osvi.dwMajorVersion, osvi.dwMinorVersion ));
#endif 
         }

      // Display service pack (if any) and build number.

         if( osvi.dwMajorVersion == 4 && 
             lstrcmpi( osvi.szCSDVersion, "Service Pack 6" ) == 0 )
         {
            HKEY hKey;
            LONG lRet;

            // Test for SP6 versus SP6a.
            lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
               "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009",
               0, KEY_QUERY_VALUE, &hKey );

#ifdef _WITH_LOG  
            if ( lRet == ERROR_SUCCESS ) {
				PrintLog((DEST, "Service Pack 6a (Build %d)", osvi.dwBuildNumber & 0xFFFF )); }
            else {
				PrintLog((DEST,"%s (Build %d)", osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF)); }
#endif 

            RegCloseKey( hKey );
         }
#ifdef _WITH_LOG  
         else // Windows NT 3.51 and earlier or Windows 2000 and later
            PrintLog((DEST, "%s (Build %d)", osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF));
#endif 


         break;

#ifdef _WITH_LOG  
      // Test for the Windows 95 product family.
      case VER_PLATFORM_WIN32_WINDOWS:

         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
         {
             PrintLog((DEST,"Microsoft Windows 95 "));
             if ( osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B' )
                PrintLog((DEST,"OSR2 " ));
         } 

         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
         {
             PrintLog((DEST,"Microsoft Windows 98 "));
             if ( osvi.szCSDVersion[1] == 'A' )
                PrintLog((DEST,"SE " ));
         } 

         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
         {
             PrintLog((DEST,"Microsoft Windows Millennium Edition"));
         } 
         break;

      case VER_PLATFORM_WIN32s:

         PrintLog((DEST,"Microsoft Win32s"));
         break;
#endif 
   }
	return true;
}

int FindKey(const char* sPluginName, const char* sDefaultKeyName, const char* sVariable, HANDLE *hKeyFile, char* output)
{
#ifdef _WITH_REGISTRY  
	char sEnvVar[BufSize];
#endif  

	DWORD rc = 0;
	char keyFile[FILENAME_SIZE];
	char sProgramFiles[FILENAME_SIZE];
	HMODULE hModule = 0;
	char pFilename[FILENAME_SIZE];
	DWORD nSize =FILENAME_SIZE;
	char path_buffer[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	
	//	HANDLE hKeyFile;
	//	char key[17];
	//	DWORD bytesRead = 0;
	
	output[0] = '\0';
	
#ifdef _WITH_LOG  
	PrintLog((DEST,"Trying to find the key file"));
#endif  

	strcat(output, "Trying to find the key file:\r\n\r\n");
	
	//*** Get the "local" version of Program Files
	GetEnvVar(PROGRAMFILES, sProgramFiles, BufSize);
	if (strlen(sProgramFiles)== 0)
	{
#ifdef _WITH_LOG  
		PrintLog((DEST,"ProgramFiles not found"));
#endif  
		return 0;
	}
	

	//default to "didn't work"
	*hKeyFile = INVALID_HANDLE_VALUE;
	
#ifdef _WITH_REGISTRY  
	//** FIRST, look for the environment variable...
#ifdef _WITH_LOG  
	PrintLog((DEST,"Looking at %s", sVariable));
#endif 
	strcat(output, "Checking for environment variable %");
	strcat(output,  sVariable);
	strcat(output, "%\r\n");
	sEnvVar[0]='\0';
	rc = GetEnvVar((char*)sVariable, sEnvVar, BufSize);
	if (strlen(sEnvVar) > 0)
	{
#ifdef _WITH_LOG  
		PrintLog((DEST,"pluginkey %s", sEnvVar));
#endif 
		strcpy(keyFile,sEnvVar);
		
		strcat(output, "Looking in pluginkey = ");
		strcat(output, sEnvVar);
		strcat(output, "\r\n");
		//open the key file
		*hKeyFile = CreateFile(sEnvVar, GENERIC_READ, FILE_SHARE_READ, 
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	
	if (*hKeyFile == INVALID_HANDLE_VALUE) 
	{
#endif 
		//** SECOND look for the key file in the "current" directory
		hModule = GetModuleHandle(sPluginName);
		rc = GetModuleFileName(hModule, (LPSTR) pFilename, nSize);
#ifdef _WITH_LOG  
		PrintLog((DEST,"GetModuleFileName %s",pFilename));
#endif 
		
 		_splitpath( pFilename, drive, dir, fname, ext );
 		_makepath(path_buffer, drive, dir,"","");

		sprintf(keyFile,"%s%s",path_buffer,sDefaultKeyName);

#ifdef _WITH_LOG  
		PrintLog((DEST,"Looking for %s",keyFile));
#endif  
		
#ifdef _WITH_REGISTRY  
		strcat(output, "Not Found.\r\n");
#endif 
		strcat(output, "Looking for:\r\n");
		strcat(output, keyFile);
		strcat(output, "\r\n");
		
		*hKeyFile = CreateFile(keyFile, GENERIC_READ, FILE_SHARE_READ, 
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#ifdef _WITH_REGISTRY  
	}
#endif  

	if (*hKeyFile == INVALID_HANDLE_VALUE) 
	{
		//** SECOND look for the key file in the "current" directory
#ifdef _WITH_LOG  
		PrintLog((DEST,"Looking for %s",sDefaultKeyName));
#endif  
		strcpy(keyFile,sDefaultKeyName);
		
		strcat(output, "Not Found.\r\n");
		strcat(output, "Looking in current directory for ");
		strcat(output, keyFile);
		strcat(output, "\r\n");
		
		*hKeyFile = CreateFile(keyFile, GENERIC_READ, FILE_SHARE_READ, 
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	
#ifdef _WITH_REGISTRY  
	if (*hKeyFile == INVALID_HANDLE_VALUE) 
	{
		//** THIRD if the key wasn't found, try Program Files\UltraVNC
#ifdef _WITH_LOG  
		PrintLog((DEST,"Looking for ProgramFiles\\UltraVNC"));
#endif  
		if (_snprintf(keyFile, sizeof(keyFile),"%s\\ULTRAVNC\\%s",sProgramFiles,sDefaultKeyName) < 0)
		{  
#ifdef _WITH_LOG  
			PrintLog((DEST,"_snprintf failed - keyFile too small"));
#endif  
		}  
		
		strcat(output, "Not Found.\r\n");
		strcat(output, "Looking in ");
		strcat(output, keyFile);
		strcat(output, "\r\n");
		*hKeyFile = CreateFile(keyFile, GENERIC_READ, FILE_SHARE_READ, 
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	
	if (*hKeyFile == INVALID_HANDLE_VALUE) 
	{
		//** LAST if the key wasn't found, try Program Files\OLR\VNC
#ifdef _WITH_LOG  
		PrintLog((DEST,"Looking for ProgramFiles\\OLR\\VNC"));
#endif  
		if (_snprintf(keyFile, sizeof(keyFile),"%s\\ORL\\VNC\\%s",sProgramFiles,sDefaultKeyName) < 0)
		{  
#ifdef _WITH_LOG  
			PrintLog((DEST,"_snprintf failed - keyFile too small"));
#endif 
		}  
		
		strcat(output, "Not Found.\r\n");
		strcat(output, "Looking in ");
		strcat(output, keyFile);
		strcat(output, "\r\n");
		
		*hKeyFile = CreateFile(keyFile, GENERIC_READ, FILE_SHARE_READ, 
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}
#endif  
	
	if (*hKeyFile == INVALID_HANDLE_VALUE)
	{
#ifdef _WITH_LOG  
		PrintLog((DEST,"KEY FILE NOT FOUND.  Using Password"));
#endif  
		strcat(output, "\r\nKEY FILE NOT FOUND.\r\n\r\nUsing Password.\r\n");
		return 0;
	}
	
#ifdef _WITH_LOG  
	PrintLog((DEST,"KEY FILE FOUND. Using key files."));
#endif  
	strcat(output, "\r\nKEY FILE FOUND.\r\n\r\nUsing key file:\r\n\r\n");
	strcat(output, keyFile);
	return 1;
	
}
