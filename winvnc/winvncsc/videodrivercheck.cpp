#include <windows.h>
#include <stdlib.h>
#ifdef DRIVER
typedef BOOL (WINAPI* pEnumDisplayDevices)(PVOID,DWORD,PVOID,DWORD);

bool CheckDriver2(void)
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
}

BOOL
IsDriverActive()
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

bool ExistDriver()
{

	LPSTR driverName = "Winvnc video hook driver";
	DEVMODE devmode;
    FillMemory(&devmode, sizeof(DEVMODE), 0);
    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;
    BOOL change = EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
    devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	BOOL result;
    DISPLAY_DEVICE dd;
    ZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(dd);
	LPSTR deviceName = NULL;
    devmode.dmDeviceName[0] = '\0';
    INT devNum = 0;
	pEnumDisplayDevices pd;
	HMODULE hUser32=LoadLibrary("USER32");
	pd = (pEnumDisplayDevices)GetProcAddress( hUser32, "EnumDisplayDevicesA");
    while (result = (*pd)(NULL,devNum, &dd,0))
        {
          if (strcmp((const char *)&dd.DeviceString[0], driverName) == 0)
              break;
           devNum++;
        }
	return result;
}
///////////////////////////////////////////////////////////////////
BOOL GetDllProductVersion(char* dllName, char *vBuffer, int size)
{
   char *versionInfo;
   void *lpBuffer;
   unsigned int cBytes;
   DWORD rBuffer;

   if( !dllName || !vBuffer )
      return(FALSE);

//   DWORD sName = GetModuleFileName(NULL, fileName, sizeof(fileName));
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

///////////////////////////////////////////////////////////////////

bool
CheckVideoDriver(bool Box)
{
	char DriverFileName1[MAX_PATH];
	char DriverFileName2[MAX_PATH];
	char DriverFileName3[MAX_PATH];
	char text [MAX_PATH*3];
	char buffer1[256];
	char buffer2[256];
	char buffer3[256];
	bool b1,b2,b3,b4,active,active2;
	b1=false;
	b2=false;
	b3=false;
	b4=false;
	active=false;
	active2=false;

	GetSystemDirectory(DriverFileName1, MAX_PATH);
	GetSystemDirectory(DriverFileName2, MAX_PATH);
	GetSystemDirectory(DriverFileName3, MAX_PATH);
	strcat(DriverFileName1,"\\vncdrv.dll");
	strcat(DriverFileName2,"\\Drivers\\vnccom.SYS");
	strcat(DriverFileName3,"\\Drivers\\vncdrv.SYS");

	GetDllProductVersion(DriverFileName1,buffer1,254);
	GetDllProductVersion(DriverFileName2,buffer2,254);
	GetDllProductVersion(DriverFileName3,buffer3,254);
	if (strcmp(buffer1,"1.00.18")==NULL) b1=true;
	if (strcmp(buffer2,"1.0.0.17")==NULL) b2=true;
	if (strcmp(buffer3,"1.00.17")==NULL) b3=true;
	if (ExistDriver()) b4=true;
	if (b1 && b2 && b3 && b4)
	{
		if (IsDriverActive()) active=true;
		if (CheckDriver2()) active2=true;
	}

	if (Box)
	{
		strcpy(text,DriverFileName1);
		strcat(text," Version:  ");
		strcat(text,buffer1);
		strcat(text,"\n");

		strcat(text,DriverFileName2);
		strcat(text," Version:  ");
		strcat(text,buffer2);
		strcat(text,"\n");

		strcat(text,DriverFileName3);
		strcat(text," Version:  ");
		strcat(text,buffer3);
		strcat(text,"\n");
		strcat(text,"\n");

		if (b4) 
		{
			strcat(text,"Driver EXIST");
			strcat(text,"\n");
		}
		else
		{
			strcat(text,"Driver NOT Installed");
			strcat(text,"\n");
		}

		if (active2) 
		{
			strcat(text,"Communication service EXIST");
			strcat(text,"\n");
		}
		else
		{
			strcat(text,"Communication service NOT Installed ");
			strcat(text,"\n");
			strcat(text,"or can not be checked (Application mode, non admin) ");
			strcat(text,"\n");
		}

		if (active) 
		{
			strcat(text,"Driver current ACTIVE");
			strcat(text,"\n");
		}
		else
		{
			strcat(text,"Driver current not ACTIVE");
			strcat(text,"\n");
		}

		MessageBox(NULL,text,"Driver Test",0);
	}

	if (b1 && b2 && b3 && b4) return true;
	else return false;
}
#endif