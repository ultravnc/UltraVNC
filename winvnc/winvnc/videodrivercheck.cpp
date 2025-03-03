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
#include <winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include "vncOSVersion.h"
#include "ScreenCapture.h"

DWORD MessageBoxSecure(HWND hWnd,LPCTSTR lpText,LPCTSTR lpCaption,UINT uType);
typedef BOOL (WINAPI* pEnumDisplayDevices)(PVOID,DWORD,PVOID,DWORD);

/*bool CheckDriver2(void)
{
	SC_HANDLE       schMaster;
	SC_HANDLE       schSlave;

	schMaster = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS); 

    schSlave = OpenService (schMaster, "vnccom", SERVICE_ALL_ACCESS);

    if (schSlave == NULL) 
    {
     CloseServiceHandle(schMaster);
	 return false;
    }
	else
	{
		CloseServiceHandle (schSlave);
		CloseServiceHandle(schMaster);
		return true;
	}
}*/


///////////////////////////////////////////////////////////////////
BOOL GetDllProductVersion(char* dllName, char *vBuffer, int size)
{
   char *versionInfo;
   void *lpBuffer;
   unsigned int cBytes;
   DWORD rBuffer;

   if( !dllName || !vBuffer )
      return(FALSE);

   DWORD sVersion = GetFileVersionInfoSize(dllName, &rBuffer);
   if (sVersion==0)
	   {
		   strcpy_s(vBuffer, 512, "Fail: Using 32-bit UltraVNC Server winvnc.exe with a 64-bit driver? \n");		   
		   return (FALSE);
		}


   versionInfo = new char[sVersion];

   GetFileVersionInfo(dllName,
                                           NULL,
                                           sVersion,
                                           versionInfo);

   BOOL resultValue = VerQueryValue(versionInfo,  

TEXT("\\StringFileInfo\\040904b0\\ProductVersion"), 
                                    &lpBuffer, 
                                    &cBytes);

   if( !resultValue )
   {
	   resultValue = VerQueryValue(versionInfo,TEXT("\\StringFileInfo\\000004b0\\ProductVersion"), 
                                    &lpBuffer, 
                                    &cBytes);

   }

   if( resultValue )
   {
      strncpy_s(vBuffer, 512, (char *) lpBuffer, size);
      delete []versionInfo;
      return(TRUE);
   }
   else
   {
      *vBuffer = '\0';
      delete []versionInfo;
      return(FALSE);
   }
}

///////////////////////////////////////////////////////////////////

bool
CheckVideoDriver(bool Box)
{
	if (IsWindows8OrGreater())
		return true;
		typedef BOOL (WINAPI* pEnumDisplayDevices)(PVOID,DWORD,PVOID,DWORD);
		pEnumDisplayDevices pd=NULL;
		LPSTR driverName = "mv video hook driver2";
		BOOL DriverFound;
		DEVMODE devmode;
		FillMemory(&devmode, sizeof(DEVMODE), 0);
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmDriverExtra = 0;
		EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
		devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		HMODULE hUser32=LoadLibrary("USER32");
		if (hUser32) pd = (pEnumDisplayDevices)GetProcAddress( hUser32, "EnumDisplayDevicesA");
		if (pd)
			{
				LPSTR deviceName=NULL;
				DISPLAY_DEVICE dd;
				ZeroMemory(&dd, sizeof(dd));
				dd.cb = sizeof(dd);
				devmode.dmDeviceName[0] = '\0';
				INT devNum = 0;
				BOOL result;
				DriverFound=false;
				while (result = (*pd)(NULL,devNum, &dd,0))
				{
					if (strcmp((const char *)&dd.DeviceString[0], driverName) == 0)
					{
					DriverFound=true;
					break;
					}
					devNum++;
				}
				if (DriverFound)
				{
					if (hUser32) FreeLibrary(hUser32);
					if(Box)
					{
						char buf[512];
						char buf2[512];
						strcpy_s(buf,"");
						strcpy_s(buf2,"");
						strcpy_s(buf2,"Driver found. \n");
						GetDllProductVersion("mv2.dll",buf,512);
						if (strcmp(buf,"1.00.22")==0)
						{
							strcat_s(buf2,"Driver version OK \n");
						}
						else
						{
							strcat_s(buf2,"Driver verion is not 1.00.22 \n");
							strcat_s(buf2,buf);
							strcat_s(buf2," \n");
						}

						if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
						{
							strcat_s(buf2," driver Active");
							HDC testdc=NULL;
							deviceName = (LPSTR)&dd.DeviceName[0];
							testdc = CreateDC("DISPLAY",deviceName,NULL,NULL);	
							if (testdc)
							{
								DeleteDC(testdc);
								strcat_s(buf2," access ok");
							}
							else
							{
								strcat_s(buf2," access denied, permission problem");
							}
						}
                        else
                        {
                            strcat_s(buf2, "Driver Not Activated, is the viewer current connected?\n");
                            strcat_s(buf2, "A Mirrior Driver can only be started if UltraVNC Server is a service or running as admin\n");
                        }
						MessageBoxSecure(NULL,buf2,buf,0);
					}
					return true;
				//deviceName = (LPSTR)&dd.DeviceName[0];
				//m_hrootdc = CreateDC("DISPLAY",deviceName,NULL,NULL);	
				//if (m_hrootdc) DeleteDC(m_hrootdc);
				}
				else if(Box) MessageBoxSecure(NULL,"Driver not found: Perhaps you need to reboot after install","Driver Info: Required version 1.00.22",0);
			}
	if (hUser32) FreeLibrary(hUser32);	
	return false;
}
