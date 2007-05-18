#ifdef DRIVER
/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
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
// http://ultravnc.sourceforge.net/
//
/////////////////////////////////////////////////////////////////////////////

#include "videodriver.h"
#include "vncOSVersion.h"z
#define MAP1 1030
#define UNMAP1 1031
#define CURSOREN 1060
#define CURSORDIS 1061



#define TESTDRIVER2 1060
#define TESTMAPPED2 1061


vncVideoDriver::vncVideoDriver()
{
	vnclog.Print(LL_INTERR, VNCLOG("vncVideodriver() \n"));
	driver_succes=false;
	blocked=false;
	hFile=NULL;
	first=true;
	Temp_Resolution=false;
}

vncVideoDriver::~vncVideoDriver()
{
	vnclog.Print(LL_INTERR, VNCLOG("~vncVideodriver() \n"));
	driver_succes=false;
	if (hFile) CloseHandle ( hFile );
	StopMirroring();
	if (IsMirrorDriverActive()) DesActivate_video_driver();
}

GETCHANGESBUF *
vncVideoDriver::RemoveCommunicationBuffer()
{
	if (hFile) CloseHandle ( hFile );
	vnclog.Print(LL_INTERR, VNCLOG(" Removed hFile \n")); 
	OutGetChangesbuf.buffer=NULL;
	OutGetChangesbuf.UserbufferBegin=NULL;
	OutGetChangesbuf.UserbufferEnd=NULL;
	return &OutGetChangesbuf;
}

GETCHANGESBUF *
vncVideoDriver::CreateCommunicationBuffer(int screensize)
{
	first=true;
	if (hFile) CloseHandle ( hFile );
	hFile = CreateFile( "\\\\.\\VncIoctl",
            GENERIC_READ | GENERIC_WRITE,
            0,
    		NULL,
    		CREATE_ALWAYS,
    		FILE_ATTRIBUTE_NORMAL,
    		NULL);

	if ( hFile == INVALID_HANDLE_VALUE ){
       vnclog.Print(LL_INTERR, VNCLOG(" Driver Communication service not found \n")); 
	   driver_succes=false;
	   return NULL;
    }
	else vnclog.Print(LL_INTERR, VNCLOG(" Driver Communication service found \n")); 
	UpdateCommunicationBuffer();
	return &OutGetChangesbuf;
}

void
vncVideoDriver::UpdateCommunicationBuffer()
{
	BOOL bRc=false;
    ULONG bytesReturned=0;
	GETCHANGESBUF InGetChangesbuf;
	InGetChangesbuf.buffer=NULL;
	InGetChangesbuf.UserbufferBegin=NULL;
	InGetChangesbuf.UserbufferEnd=NULL;
	bRc = DeviceIoControl ( hFile, 
					(DWORD) IOCTL_SIOCTL_METHOD_BUFFERED, 
					&InGetChangesbuf, 
					sizeof(GETCHANGESBUF),
					&OutGetChangesbuf,
                    sizeof(GETCHANGESBUF),
                	&bytesReturned,
					NULL 
					);
	if ( !bRc )
	{
		vnclog.Print(LL_INTERR, VNCLOG("Error in DeviceIoControl \n")); 
	}

}

void
vncVideoDriver::StartMirroring()
{
	first=true;
	vnclog.Print(LL_INTERR, VNCLOG("StartMirroring() \n"));
	gdc = GetDC(NULL);
	oldaantal=1;
	driver_succes=false;
	if (ExtEscape(gdc, MAP1, 0, NULL, NULL, NULL)>0) driver_succes=true;
	HardwareCursor();
	ReleaseDC(NULL,gdc);
	// We need also to check if the communication service is activer
	if (!hFile)
	{
		hFile = CreateFile( "\\\\.\\VncIoctl",
            GENERIC_READ | GENERIC_WRITE,
            0,
    		NULL,
    		CREATE_ALWAYS,
    		FILE_ATTRIBUTE_NORMAL,
    		NULL);

		if ( hFile == INVALID_HANDLE_VALUE ){
			driver_succes=false;
		}
		else
		{
			CloseHandle ( hFile );
			hFile=NULL;
		}

	}

}

BOOL
vncVideoDriver::HardwareCursor()
{
	gdc = GetDC(NULL);
	BOOL driver2=false;
	int returnvalue;
	returnvalue= ExtEscape(gdc, CURSOREN, 0, NULL, NULL, NULL);
	ReleaseDC(NULL,gdc);
	if (returnvalue>0) driver2=true;
	return driver2;
}

BOOL
vncVideoDriver::NoHardwareCursor()
{
	gdc = GetDC(NULL);
	BOOL driver2=false;
	int returnvalue;
	returnvalue= ExtEscape(gdc, CURSORDIS, 0, NULL, NULL, NULL);
	ReleaseDC(NULL,gdc);
	if (returnvalue>0) driver2=true;
	return driver2;
}


void
vncVideoDriver::StopMirroring()
{
	driver_succes=false;
	vnclog.Print(LL_INTERR, VNCLOG("StopMirroring() \n"));
	gdc = GetDC(NULL);
	ExtEscape(gdc, UNMAP1, 0, NULL, NULL, NULL);
	ReleaseDC(NULL, gdc);
	first=true;
}



BOOL
vncVideoDriver::Activate_video_driver(bool auto,int x,int y, int w,int h)
   {
	#define DM_POSITION             0x00000020L
	first=true;
	vnclog.Print(LL_INTERR, VNCLOG("Activate_video_driver() \n"));
	HDESK   hdeskInput;
    HDESK   hdeskCurrent;
    int j=0;
	pEnumDisplayDevices pd;
	pChangeDisplaySettingsExA pcdse;
	LPSTR driverName = "Winvnc video hook driver";
	DEVMODE devmode;
    FillMemory(&devmode, sizeof(DEVMODE), 0);
    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;
    BOOL change = EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
    devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT|DM_POSITION|DM_DISPLAYFLAGS ;
	HMODULE hUser32=LoadLibrary("USER32");
	pd = (pEnumDisplayDevices)GetProcAddress( hUser32, "EnumDisplayDevicesA");
	pcdse = (pChangeDisplaySettingsExA)GetProcAddress( hUser32, "ChangeDisplaySettingsExA");
	if (!pcdse) return false;
    if (pd)
    {
        DISPLAY_DEVICE dd;
        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);
		LPSTR deviceName = NULL;
        devmode.dmDeviceName[0] = '\0';
        INT devNum = 0;
        BOOL result;
        while (result = (*pd)(NULL,devNum, &dd,0))
        {
          if (strcmp((const char *)&dd.DeviceString[0], driverName) == 0)
              break;

           devNum++;
        }
       
        if (!result)
        {
           printf("No '%s' found.\n", driverName);
           return FALSE;
        }

//        printf("DevNum:%d\nName:%s\nString:%s\n\n",devNum,&dd.DeviceName[0],&dd.DeviceString[0]);
//		printf("Sreen Settings'%i %i %i'\n",devmode.dmPelsWidth,devmode.dmPelsHeight,devmode.dmBitsPerPel);


		CHAR deviceNum[MAX_PATH];
        strcpy(&deviceNum[0], "DEVICE0");

        HKEY hKeyProfileMirror = (HKEY)0;
        if (RegCreateKey(HKEY_LOCAL_MACHINE,
                        ("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\vncdrv"),
                         &hKeyProfileMirror) != ERROR_SUCCESS)
        {
//           printf("Can't access registry.\n");
           return FALSE;
        }

        HKEY hKeyDevice = (HKEY)0;
        if (RegCreateKey(hKeyProfileMirror,
                         (&deviceNum[0]),
                         &hKeyDevice) != ERROR_SUCCESS)
        {
//           printf("Can't access DEVICE# hardware profiles key.\n");
           return FALSE;
        }

        DWORD one = 1;
        if (RegSetValueEx(hKeyDevice,
                          ("Attach.ToDesktop"),
                          0,
                          REG_DWORD,
                          (unsigned char *)&one,
                          4) != ERROR_SUCCESS)
        {
//           printf("Can't set Attach.ToDesktop to 0x1\n");
           return FALSE;
        }

        strcpy((LPSTR)&devmode.dmDeviceName[0], "vncdrv");
        deviceName = (LPSTR)&dd.DeviceName[0];
		// save the current desktop
		// save the current desktop

		hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
		if (hdeskCurrent != NULL)
			{
				DWORD dwflag=0;
				hdeskInput = OpenInputDesktop(dwflag, FALSE, MAXIMUM_ALLOWED);
				if (hdeskInput != NULL)
					{
						SetThreadDesktop(hdeskInput);
					}
				else Beep(1000,500);
			}
//		BOOL change = EnumDisplaySettings(deviceName,ENUM_CURRENT_SETTINGS,&devmode);
		devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT|DM_POSITION|DM_DISPLAYFLAGS ;
		if (devmode.dmBitsPerPel==8 || devmode.dmBitsPerPel==16 || devmode.dmBitsPerPel==32) 
		{
			devmode.dmPelsWidth=w;
			devmode.dmPelsHeight=h;
			devmode.dmPosition.x=x;
			devmode.dmPosition.y=y;
	
			// add 'Default.*' settings to the registry under above hKeyProfile\mirror\device
			POINT CursorPos;
			GetCursorPos(&CursorPos);
			INT code =
			(*pcdse)(deviceName,
									&devmode, 
									NULL,
									CDS_UPDATEREGISTRY,
									NULL
									);
			SetCursorPos(CursorPos.x,CursorPos.y);
			{
				DEVMODE devmode;
				FillMemory(&devmode, sizeof(DEVMODE), 0);
				devmode.dmSize = sizeof(DEVMODE);
				devmode.dmDriverExtra = 0;
				devmode.dmFields = DM_POSITION;
				devmode.dmPosition.x=x;
				devmode.dmPosition.y=y;
			}
			// reset desktop
			SetThreadDesktop(hdeskCurrent);


		   // close the input desktop
			CloseDesktop(hdeskInput);

			DWORD zero = 1;
			if (RegSetValueEx(hKeyDevice,
                          ("Attach.ToDesktop"),
                          0,
                          REG_DWORD,
                          (unsigned char *)&zero,
                          4) != ERROR_SUCCESS)
				{
		//           printf("Can't set Attach.ToDesktop to 0x0\n");
						return 1;
				}

			RegCloseKey(hKeyProfileMirror);
			RegCloseKey(hKeyDevice);
		}
		else
		{
			// reset desktop
			SetThreadDesktop(hdeskCurrent);


		   // close the input desktop
			CloseDesktop(hdeskInput);

			DWORD zero = 0;
			if (RegSetValueEx(hKeyDevice,
                          ("Attach.ToDesktop"),
                          0,
                          REG_DWORD,
                          (unsigned char *)&zero,
                          4) != ERROR_SUCCESS)
				{
		//           printf("Can't set Attach.ToDesktop to 0x0\n");
						return 1;
				}

			RegCloseKey(hKeyProfileMirror);
			RegCloseKey(hKeyDevice);
		}
		}
	 if (hUser32) FreeLibrary(hUser32);
   Sleep(500);
   return TRUE;
   }


void
vncVideoDriver::DesActivate_video_driver()
   {
	first=true;
	vnclog.Print(LL_INTERR, VNCLOG("DesActivate_video_driver() \n"));
	HDESK   hdeskInput;
    HDESK   hdeskCurrent;
    int j=0;
	pEnumDisplayDevices pd;
	pChangeDisplaySettingsExA pcdse;
	LPSTR driverName = "Winvnc video hook driver";
	DEVMODE devmode;
    FillMemory(&devmode, sizeof(DEVMODE), 0);
    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;
    BOOL change = EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
    devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	HMODULE hUser32=LoadLibrary("USER32");
	pd = (pEnumDisplayDevices)GetProcAddress( hUser32, "EnumDisplayDevicesA");
	pcdse = (pChangeDisplaySettingsExA)GetProcAddress( hUser32, "ChangeDisplaySettingsExA");
	if (!pcdse) return;
    if (pd)
    {
        DISPLAY_DEVICE dd;
        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);
		LPSTR deviceName = NULL;
        devmode.dmDeviceName[0] = '\0';
        INT devNum = 0;
        BOOL result;
        while (result = (*pd)(NULL,devNum, &dd,0))
        {
          if (strcmp((const char *)&dd.DeviceString[0], driverName) == 0)
              break;

           devNum++;
        }
       
        if (!result)
        {
//           printf("No '%s' found.\n", driverName);
           return;
        }

//        printf("DevNum:%d\nName:%s\nString:%s\n\n",devNum,&dd.DeviceName[0],&dd.DeviceString[0]);
//		printf("Sreen Settings'%i %i %i'\n",devmode.dmPelsWidth,devmode.dmPelsHeight,devmode.dmBitsPerPel);


		CHAR deviceNum[MAX_PATH];
        strcpy(&deviceNum[0], "DEVICE0");

        HKEY hKeyProfileMirror = (HKEY)0;
        if (RegCreateKey(HKEY_LOCAL_MACHINE,
                        ("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\vncdrv"),
                         &hKeyProfileMirror) != ERROR_SUCCESS)
        {
//           printf("Can't access registry.\n");
           return;
        }

        HKEY hKeyDevice = (HKEY)0;
        if (RegCreateKey(hKeyProfileMirror,
                         (&deviceNum[0]),
                         &hKeyDevice) != ERROR_SUCCESS)
        {
//           printf("Can't access DEVICE# hardware profiles key.\n");
           return;
        }

        DWORD one = 0;
        if (RegSetValueEx(hKeyDevice,
                          ("Attach.ToDesktop"),
                          0,
                          REG_DWORD,
                          (unsigned char *)&one,
                          4) != ERROR_SUCCESS)
        {
//           printf("Can't set Attach.ToDesktop to 0x1\n");
           return;
        }

        strcpy((LPSTR)&devmode.dmDeviceName[0], "vncdrv");
        deviceName = (LPSTR)&dd.DeviceName[0];
		// save the current desktop
		// save the current desktop

		hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
		if (hdeskCurrent != NULL)
			{
				hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
				if (hdeskInput != NULL)
					{
						SetThreadDesktop(hdeskInput);
					}
				else Beep(1000,500);
			}
        // add 'Default.*' settings to the registry under above hKeyProfile\mirror\device
		blocked=true;
if (!Temp_Resolution)
{
        INT code =
         (*pcdse)(deviceName,
                                &devmode, 
                                NULL,
                                CDS_UPDATEREGISTRY,
                                NULL
                                );
    
//        printf("Update Register on device mode: %i\n", code);

        code = (*pcdse)(deviceName,
                                &devmode, 
                                NULL,
                                0,
                                NULL
                                );
}

		blocked=false;
		if (hUser32)FreeLibrary(hUser32);	

   // reset desktop
   SetThreadDesktop(hdeskCurrent);


   // close the input desktop
   CloseDesktop(hdeskInput);

		DWORD zero = 0;
        if (RegSetValueEx(hKeyDevice,
                          ("Attach.ToDesktop"),
                          0,
                          REG_DWORD,
                          (unsigned char *)&zero,
                          4) != ERROR_SUCCESS)
        {
//           printf("Can't set Attach.ToDesktop to 0x0\n");
           return;
        }

        RegCloseKey(hKeyProfileMirror);
        RegCloseKey(hKeyDevice);

      }
   return;
   }







BOOL
vncVideoDriver::ExistMirrorDriver()
{
pEnumDisplayDevices pd;
	LPSTR driverName = "Winvnc video hook driver";
	DEVMODE devmode;
    FillMemory(&devmode, sizeof(DEVMODE), 0);
    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;
    BOOL change = EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
    devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	HMODULE hUser32=LoadLibrary("USER32");
	pd = (pEnumDisplayDevices)GetProcAddress( hUser32, "EnumDisplayDevicesA");
	BOOL result;


    if (pd)
    {
        DISPLAY_DEVICE dd;
        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);
		LPSTR deviceName = NULL;
        devmode.dmDeviceName[0] = '\0';
        INT devNum = 0;
        while (result = (*pd)(NULL,devNum, &dd,0))
        {
          if (strcmp((const char *)&dd.DeviceString[0], driverName) == 0)
              break;

           devNum++;
        }
		if (hUser32) FreeLibrary(hUser32);
	return result;
	}
	else return 0;
}

HDC
vncVideoDriver::GetDcMirror()
{
typedef BOOL (WINAPI* pEnumDisplayDevices)(PVOID,DWORD,PVOID,DWORD);
		HDC m_hrootdc;
		pEnumDisplayDevices pd;
		LPSTR driverName = "Winvnc video hook driver";
		BOOL DriverFound;
		DEVMODE devmode;
		FillMemory(&devmode, sizeof(DEVMODE), 0);
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmDriverExtra = 0;
		BOOL change = EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
		devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		HMODULE hUser32=LoadLibrary("USER32");
		pd = (pEnumDisplayDevices)GetProcAddress( hUser32, "EnumDisplayDevicesA");
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
				deviceName = (LPSTR)&dd.DeviceName[0];
				m_hrootdc = CreateDC("DISPLAY",deviceName,NULL,NULL);				
				}
			}
		if (hUser32) FreeLibrary(hUser32);

		return m_hrootdc;
}



BOOL
vncVideoDriver::IsMirrorDriverActive()
{
pEnumDisplayDevices pd;
	LPSTR driverName = "Winvnc video hook driver";
	DEVMODE devmode;
    FillMemory(&devmode, sizeof(DEVMODE), 0);
    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;
    BOOL change = EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
    devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	HMODULE hUser32=LoadLibrary("USER32");
	pd = (pEnumDisplayDevices)GetProcAddress( hUser32, "EnumDisplayDevicesA");
	BOOL result;


    if (pd)
    {
        DISPLAY_DEVICE dd;
        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);
		LPSTR deviceName = NULL;
        devmode.dmDeviceName[0] = '\0';
        INT devNum = 0;
        while (result = (*pd)(NULL,devNum, &dd,0))
        {
          if (strcmp((const char *)&dd.DeviceString[0], driverName) == 0)
              break;

           devNum++;
        }
	if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
		{
			if (hUser32) FreeLibrary(hUser32);
			return 1;
		}
	else 
		{
			if (hUser32) FreeLibrary(hUser32);
			return 0;
		}
	}
	return 0;
}

BOOL
vncVideoDriver:: GetDllProductVersion(char* dllName, char *vBuffer, int size)
{
   char *versionInfo;
   char fileName[MAX_PATH + 1];
   void *lpBuffer;
   unsigned int cBytes;
   DWORD rBuffer;

   if( !dllName || !vBuffer )
      return(FALSE);

   DWORD sName = GetModuleFileName(NULL, fileName, sizeof(fileName));
// FYI only
   DWORD sVersion = GetFileVersionInfoSize(dllName, &rBuffer);
   if (sVersion==0) return (FALSE);
   versionInfo = new char[sVersion];

   BOOL resultVersion = GetFileVersionInfo(dllName,
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
      strncpy(vBuffer, (char *) lpBuffer, size);
      delete versionInfo;
      return(TRUE);
   }
   else
   {
      *vBuffer = '\0';
      delete versionInfo;
      return(FALSE);
   }
}

BOOL
vncVideoDriver::Tempres()
{
	pEnumDisplayDevices pd;
	HMODULE hUser32=LoadLibrary("USER32");
	pd = (pEnumDisplayDevices)GetProcAddress( hUser32, "EnumDisplayDevicesA");

	first=true;
	DWORD i;
	int curw,curh,curp;
	int regw,regh,regp;
	DISPLAY_DEVICE dd;
	DEVMODE dm;
	LPSTR deviceName = NULL;
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	DEVMODE devmode;
	FillMemory(&devmode, sizeof(DEVMODE), 0);
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;

	for(i=0; (*pd)(NULL, i, &dd, 0); i++)
		{
			if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
				{
					ZeroMemory(&dm, sizeof(dm));
					dm.dmSize = sizeof(dm);
					EnumDisplaySettings((char*)(dd.DeviceName), ENUM_CURRENT_SETTINGS, &dm);
					break;
				}
		}

	deviceName = (LPSTR)&dd.DeviceName[0];			
	EnumDisplaySettings(deviceName,ENUM_CURRENT_SETTINGS,&devmode);
	curw=devmode.dmPelsWidth;
	curh=devmode.dmPelsHeight;
	curp=devmode.dmBitsPerPel;
	EnumDisplaySettings(deviceName,ENUM_REGISTRY_SETTINGS,&devmode);
	regw=devmode.dmPelsWidth;
	regh=devmode.dmPelsHeight;
	regp=devmode.dmBitsPerPel;
	if (curw==regw && curh==regh && curp==regp)
		return false;
	else return true;
}



#endif